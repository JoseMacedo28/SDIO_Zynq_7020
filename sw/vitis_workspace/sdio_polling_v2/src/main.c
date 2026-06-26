#include <stdio.h>
#include <string.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "ff.h"
#include "xsdps.h"
#include "xsdps_hw.h"

/* ─── Objetos globales ─────────────────────────────── */
static FATFS fatfs;
static FIL   fil;
static FRESULT fr;

/* ─── Buffers ──────────────────────────────────────── */
#define BUF_SIZE 1024
static char write_buf[BUF_SIZE];
static char read_buf[BUF_SIZE];

/* ─── Prototipos ───────────────────────────────────── */
void verificar_clock(void);
FRESULT sd_borrar_recursivo(const char *path);
void sd_limpiar_raiz(void);
void sd_crear_carpetas(void);
void test_txt(void);
void test_bin(void);
void test_json(void);
void leer_archivo(const char *path);

/* ════════════════════════════════════════════════════ */
int main(void)
{
    xil_printf("\r\n=== SDIO Polling Test v2 ===\r\n");

    verificar_clock();

    fr = f_mount(&fatfs, "0:", 1);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_mount (%d)\r\n", fr);
        return -1;
    }
    xil_printf("SD montada\r\n");

    sd_limpiar_raiz();
    sd_crear_carpetas();
    test_txt();
    test_bin();
    test_json();

    f_unmount("0:");
    xil_printf("\r\n=== Test finalizado ===\r\n");

    return 0;
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

    xil_printf("CLK_CTRL:   0x%08X\r\n", (unsigned int)clk_ctrl);
    xil_printf("Base clock: %d MHz\r\n", (int)(base_clk / 1000000));
    xil_printf("Divisor:    %d %s\r\n", (int)divisor,
               divisor == 0 ? "(sin preescalador)" : "");
    xil_printf("SD Clock:   %d MHz\r\n", (int)(freq_hz / 1000000));
    xil_printf("BusWidth:   %d bits\r\n",
               (int)(SdInstance.BusWidth == 2 ? 4 : 1));
    xil_printf("CardType:   %s\r\n",
               SdInstance.CardType == XSDPS_CARD_SD ? "SD" : "otros");
    xil_printf("Verificado: 50 MHz (osciloscopio pin CLK)\r\n");
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

/* ─── Limpiar toda la SD ───────────────────────────── */
void sd_limpiar_raiz(void)
{
    xil_printf("\r\n-- Limpiando SD completa --\r\n");
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
    else
        xil_printf("  ERROR /texto (%d)\r\n", fr);

    fr = f_mkdir("0:/binarios");
    if (fr == FR_OK || fr == FR_EXIST)
        xil_printf("  OK: /binarios\r\n");
    else
        xil_printf("  ERROR /binarios (%d)\r\n", fr);

    fr = f_mkdir("0:/jdata");
    if (fr == FR_OK || fr == FR_EXIST)
        xil_printf("  OK: /jdata\r\n");
    else
        xil_printf("  ERROR /jdata (%d)\r\n", fr);
}

/* ─── Test .txt ────────────────────────────────────── */
void test_txt(void)
{
    UINT bw;
    xil_printf("\r\n-- Test .txt --\r\n");

    snprintf(write_buf, BUF_SIZE,
        "Archivo de texto desde Zybo Z7-20\r\n"
        "Protocolo: SDIO\r\n"
        "Modo: Polling\r\n"
        "Bus: 4-bit, 50 MHz\r\n");

    fr = f_open(&fil, "0:/texto/datos.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }

    f_write(&fil, write_buf, strlen(write_buf), &bw);
    f_close(&fil);
    xil_printf("  Escrito: %d bytes\r\n", (int)bw);

    leer_archivo("0:/texto/datos.txt");
}

/* ─── Test .bin ────────────────────────────────────── */
void test_bin(void)
{
    UINT bw, br;
    u8 bin_buf[256];
    u32 i;
    int errores = 0;

    xil_printf("\r\n-- Test .bin --\r\n");

    for (i = 0; i < 256; i++) bin_buf[i] = (u8)i;

    fr = f_open(&fil, "0:/binarios/datos.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }
    f_write(&fil, bin_buf, 256, &bw);
    f_close(&fil);
    xil_printf("  Escrito: %d bytes\r\n", (int)bw);

    memset(bin_buf, 0, 256);
    fr = f_open(&fil, "0:/binarios/datos.bin", FA_READ);
    if (fr != FR_OK) { xil_printf("ERROR open lectura (%d)\r\n", fr); return; }
    f_read(&fil, bin_buf, 256, &br);
    f_close(&fil);

    for (i = 0; i < 256; i++)
        if (bin_buf[i] != (u8)i) errores++;

    if (errores == 0)
        xil_printf("  OK: %d bytes, patron 0x00-0xFF correcto\r\n", (int)br);
    else
        xil_printf("  ERROR: %d bytes incorrectos\r\n", errores);
}

/* ─── Test .json ───────────────────────────────────── */
void test_json(void)
{
    UINT bw;
    xil_printf("\r\n-- Test .json --\r\n");

    snprintf(write_buf, BUF_SIZE,
        "{\r\n"
        "  \"dispositivo\": \"Zybo Z7-20\",\r\n"
        "  \"protocolo\": \"SDIO\",\r\n"
        "  \"modo\": \"polling\",\r\n"
        "  \"bus_bits\": 4,\r\n"
        "  \"clock_mhz\": 50,\r\n"
        "  \"estado\": \"OK\"\r\n"
        "}\r\n");

    fr = f_open(&fil, "0:/jdata/config.json", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) { xil_printf("ERROR open (%d)\r\n", fr); return; }

    f_write(&fil, write_buf, strlen(write_buf), &bw);
    f_close(&fil);
    xil_printf("  Escrito: %d bytes\r\n", (int)bw);

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