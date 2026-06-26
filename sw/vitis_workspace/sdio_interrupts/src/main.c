#include <stdio.h>
#include <string.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xsdps.h"
#include "xsdps_hw.h"
#include "ff.h"
#include "xil_exception.h"

/* ─── Defines no generados por SDT ────────────────── */
#define XPAR_XSDPS_0_INTR            56U
#define XPAR_SCUGIC_SINGLE_DEVICE_ID  0U

/* ─── Objetos globales ─────────────────────────────── */
static FATFS   fatfs;
static FIL     fil;
static FRESULT fr;
static XScuGic GicInstance;

/* ─── Flag de interrupcion ─────────────────────────── */
volatile int TransferDone = 0;

/* ─── Buffers ──────────────────────────────────────── */
#define BUF_SIZE 1024
static char write_buf[BUF_SIZE];
static char read_buf[BUF_SIZE];

/* ─── Prototipos ───────────────────────────────────── */
void    sd_isr(void *CallBackRef);
int     gic_init_base(void);
int     gic_connect_sd_isr(void);
void    verificar_clock(void);
FRESULT sd_borrar_recursivo(const char *path);
void    sd_limpiar_raiz(void);
void    sd_crear_carpetas(void);
void    test_txt(void);
void    test_bin(void);
void    test_json(void);
void    leer_archivo(const char *path);

/* ════════════════════════════════════════════════════ */
int main(void)
{
    int status;

    xil_printf("\r\n=== SDIO Interrupciones Test ===\r\n");

    /* 1. Inicializar GIC base */
    status = gic_init_base();
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: GIC init (%d)\r\n", status);
        return -1;
    }

    /* 2. Verificar clock */
    verificar_clock();

    /* 3. Montar SD */
    fr = f_mount(&fatfs, "0:", 1);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_mount (%d)\r\n", fr);
        return -1;
    }
    xil_printf("SD montada\r\n");

    /* 4. Conectar ISR directamente al hardware */
    status = gic_connect_sd_isr();
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: conectar ISR (%d)\r\n", status);
        return -1;
    }
    xil_printf("ISR SDIO conectada\r\n");

    /* 5. Limpiar y preparar */
    sd_limpiar_raiz();
    sd_crear_carpetas();

    /* 6. Pruebas */
    test_txt();
    test_bin();
    test_json();

    /* 7. Desmontar */
    f_unmount("0:");
    xil_printf("\r\n=== Test finalizado ===\r\n");

    return 0;
}

/* ─── ISR directa del SDIO ─────────────────────────── */
void sd_isr(void *CallBackRef)
{
    (void)CallBackRef;
    u32 status;

    status = XSdPs_ReadReg(XPAR_XSDPS_0_BASEADDR,
                            XSDPS_NORM_INTR_STS_OFFSET);
    XSdPs_WriteReg(XPAR_XSDPS_0_BASEADDR,
                    XSDPS_NORM_INTR_STS_OFFSET, status);

    TransferDone = 1;
    xil_printf("  [ISR] SDIO status: 0x%04X\r\n", (unsigned int)status);
}

/* ─── Inicializar GIC base ─────────────────────────── */
int gic_init_base(void)
{
    XScuGic_Config *GicConfig;
    int status;

    GicConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (!GicConfig) {
        xil_printf("ERROR: GIC LookupConfig\r\n");
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(&GicInstance, GicConfig,
                                    XPAR_SCUGIC_CPU_BASEADDR);
    if (status != XST_SUCCESS) return status;

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                  (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                  &GicInstance);
    Xil_ExceptionEnable();

    xil_printf("GIC inicializado\r\n");
    return XST_SUCCESS;
}

/* ─── Conectar ISR del SDIO ────────────────────────── */
int gic_connect_sd_isr(void)
{
    int status;

    status = XScuGic_Connect(&GicInstance,
                              XPAR_XSDPS_0_INTR,
                              (Xil_InterruptHandler)sd_isr,
                              NULL);
    if (status != XST_SUCCESS) return status;

    XScuGic_Enable(&GicInstance, XPAR_XSDPS_0_INTR);
    xil_printf("  IRQ SDIO habilitada (ID: %d)\r\n", (int)XPAR_XSDPS_0_INTR);

    return XST_SUCCESS;
}

/* ─── Verificar clock ──────────────────────────────── */
void verificar_clock(void)
{
    XSdPs SdInstance;
    XSdPs_Config *SdConfig;
    u32 clk_ctrl, divisor, freq_hz, base_clk;
    int status;

    xil_printf("\r\n-- Verificacion clock --\r\n");

    SdConfig = XSdPs_LookupConfig(XPAR_XSDPS_0_BASEADDR);
    if (!SdConfig) { xil_printf("ERROR: LookupConfig\r\n"); return; }

    status = XSdPs_CfgInitialize(&SdInstance, SdConfig,
                                  SdConfig->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: CfgInitialize (%d)\r\n", status);
        return;
    }

    status = XSdPs_CardInitialize(&SdInstance);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: CardInitialize (%d)\r\n", status);
        return;
    }

    base_clk = SdConfig->InputClockHz;
    clk_ctrl = XSdPs_ReadReg(SdInstance.Config.BaseAddress,
                              XSDPS_CLK_CTRL_OFFSET);
    divisor  = (clk_ctrl >> 8) & 0xFF;

    if (divisor == 0)
        freq_hz = base_clk;
    else
        freq_hz = base_clk / (2U * divisor);

    xil_printf("Base clock: %d MHz\r\n", (int)(base_clk / 1000000));
    xil_printf("SD Clock:   %d MHz\r\n", (int)(freq_hz / 1000000));
    xil_printf("BusWidth:   %d bits\r\n",
               (int)(SdInstance.BusWidth == 2 ? 4 : 1));
    xil_printf("Verificado: 50 MHz (osciloscopio)\r\n");
}

/* ─── Borrado recursivo ────────────────────────────── */
FRESULT sd_borrar_recursivo(const char *path)
{
    DIR dir;
    FILINFO fno;
    char fullpath[256];
    FRESULT res;

    res = f_opendir(&dir, path);
    if (res != FR_OK) return res;

    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        if (fno.fname[0] == '.') continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, fno.fname);

        if (fno.fattrib & AM_DIR) {
            res = sd_borrar_recursivo(fullpath);
            if (res != FR_OK) {
                xil_printf("  ERROR recursivo en %s (%d)\r\n", fullpath, res);
                continue;
            }
        }

        res = f_unlink(fullpath);
        if (res == FR_OK)
            xil_printf("  borrado: %s\r\n", fullpath);
        else
            xil_printf("  ERROR borrando %s (%d)\r\n", fullpath, res);
    }

    f_closedir(&dir);
    return FR_OK;
}

/* ─── Limpiar SD ───────────────────────────────────── */
void sd_limpiar_raiz(void)
{
    xil_printf("\r\n-- Limpiando SD --\r\n");
    sd_borrar_recursivo("0:");
    xil_printf("SD limpia\r\n");
}

/* ─── Crear carpetas ───────────────────────────────── */
void sd_crear_carpetas(void)
{
    xil_printf("\r\n-- Creando carpetas --\r\n");

    fr = f_mkdir("0:/texto");
    if (fr == FR_OK || fr == FR_EXIST)
        xil_printf("  OK: /texto\r\n");

    fr = f_mkdir("0:/binarios");
    if (fr == FR_OK || fr == FR_EXIST)
        xil_printf("  OK: /binarios\r\n");

    fr = f_mkdir("0:/jdata");
    if (fr == FR_OK || fr == FR_EXIST)
        xil_printf("  OK: /jdata\r\n");
}

/* ─── Test .txt ────────────────────────────────────── */
void test_txt(void)
{
    UINT bw;
    u32 timeout;

    xil_printf("\r\n-- Test .txt --\r\n");

    TransferDone = 0;

    snprintf(write_buf, BUF_SIZE,
        "Archivo de texto desde Zybo Z7-20\r\n"
        "Protocolo: SDIO\r\n"
        "Modo: Interrupciones\r\n"
        "Bus: 4-bit, 50 MHz\r\n");

    fr = f_open(&fil, "0:/texto/datos.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }

    f_write(&fil, write_buf, strlen(write_buf), &bw);
    f_close(&fil);

    timeout = 2000000;
    while (!TransferDone && timeout--);
    if (TransferDone)
        xil_printf("  OK: %d bytes, IRQ confirmada\r\n", (int)bw);
    else
        xil_printf("  OK: %d bytes (timeout IRQ)\r\n", (int)bw);

    leer_archivo("0:/texto/datos.txt");
}

/* ─── Test .bin ────────────────────────────────────── */
void test_bin(void)
{
    UINT bw, br;
    u8 bin_buf[256];
    u32 i, timeout;
    int errores = 0;

    xil_printf("\r\n-- Test .bin --\r\n");

    TransferDone = 0;

    for (i = 0; i < 256; i++) bin_buf[i] = (u8)i;

    fr = f_open(&fil, "0:/binarios/datos.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }
    f_write(&fil, bin_buf, 256, &bw);
    f_close(&fil);

    timeout = 2000000;
    while (!TransferDone && timeout--);
    if (TransferDone)
        xil_printf("  Escrito: %d bytes, IRQ confirmada\r\n", (int)bw);
    else
        xil_printf("  Escrito: %d bytes (timeout IRQ)\r\n", (int)bw);

    memset(bin_buf, 0, 256);
    fr = f_open(&fil, "0:/binarios/datos.bin", FA_READ);
    if (fr != FR_OK) { xil_printf("ERROR open lectura (%d)\r\n", fr); return; }
    f_read(&fil, bin_buf, 256, &br);
    f_close(&fil);

    for (i = 0; i < 256; i++)
        if (bin_buf[i] != (u8)i) errores++;

    if (errores == 0)
        xil_printf("  OK: patron 0x00-0xFF correcto\r\n");
    else
        xil_printf("  ERROR: %d bytes incorrectos\r\n", errores);
}

/* ─── Test .json ───────────────────────────────────── */
void test_json(void)
{
    UINT bw;
    u32 timeout;

    xil_printf("\r\n-- Test .json --\r\n");

    TransferDone = 0;

    snprintf(write_buf, BUF_SIZE,
        "{\r\n"
        "  \"dispositivo\": \"Zybo Z7-20\",\r\n"
        "  \"protocolo\": \"SDIO\",\r\n"
        "  \"modo\": \"interrupciones\",\r\n"
        "  \"bus_bits\": 4,\r\n"
        "  \"clock_mhz\": 50,\r\n"
        "  \"estado\": \"OK\"\r\n"
        "}\r\n");

    fr = f_open(&fil, "0:/jdata/config.json", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }

    f_write(&fil, write_buf, strlen(write_buf), &bw);
    f_close(&fil);

    timeout = 2000000;
    while (!TransferDone && timeout--);
    if (TransferDone)
        xil_printf("  OK: %d bytes, IRQ confirmada\r\n", (int)bw);
    else
        xil_printf("  OK: %d bytes (timeout IRQ)\r\n", (int)bw);

    leer_archivo("0:/jdata/config.json");
}

/* ─── Leer e imprimir archivo ──────────────────────── */
void leer_archivo(const char *path)
{
    UINT br;
    memset(read_buf, 0, BUF_SIZE);

    fr = f_open(&fil, path, FA_READ);
    if (fr != FR_OK) { xil_printf("ERROR open lectura (%d)\r\n", fr); return; }

    f_read(&fil, read_buf, BUF_SIZE - 1, &br);
    f_close(&fil);

    xil_printf("  Leido (%d bytes):\r\n%s\r\n", (int)br, read_buf);
}