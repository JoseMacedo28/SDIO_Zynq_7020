#include <stdio.h>
#include <string.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "ff.h"
#include "xsdps_hw.h"
#include "xsdps.h"

/* ─── Objetos globales ─────────────────────────────── */
static FATFS fatfs;
static FIL   fil;
static FRESULT fr;

/* ─── Buffers ──────────────────────────────────────── */
#define BUF_SIZE 512
static char write_buf[BUF_SIZE];
static char read_buf[BUF_SIZE];

/* ─── Prototipos ───────────────────────────────────── */
void verificar_clock(void);
void test_write_txt(void);
void test_read_txt(void);
void test_write_bin(void);

/* ════════════════════════════════════════════════════ */
int main(void)
{
    xil_printf("\r\n=== SDIO Polling Test ===\r\n");

    /* Verificar clock ANTES de FatFs */
    verificar_clock_init();

    /* Ahora sí montar FatFs */
    fr = f_mount(&fatfs, "0:", 1);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_mount (%d)\r\n", fr);
        return -1;
    }
    xil_printf("SD montada correctamente\r\n");

    test_write_txt();
    test_read_txt();
    test_write_bin();

    f_unmount("0:");
    xil_printf("\r\n=== Test finalizado ===\r\n");

    return 0;
}

/* ─── Verificar clock real del controlador SDIO ────── */
void verificar_clock_init(void)
{
    XSdPs SdInstance;
    XSdPs_Config *SdConfig;
    u32 clk_ctrl, divisor, freq_hz, base_clk;
    int status;

    xil_printf("\r\n-- Verificacion clock post-init --\r\n");

    SdConfig = XSdPs_LookupConfig(XPAR_XSDPS_0_BASEADDR);
    if (!SdConfig) {
        xil_printf("ERROR: LookupConfig\r\n");
        return;
    }

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

    /* Leer clock base del config — viene del BSP/xparameters */
    base_clk = SdConfig->InputClockHz;

    /* Leer registro del controlador */
    clk_ctrl = XSdPs_ReadReg(SdInstance.Config.BaseAddress,
                              XSDPS_CLK_CTRL_OFFSET);
    divisor  = (clk_ctrl >> 8) & 0xFF;

    if (divisor == 0)
    freq_hz = base_clk;        /* sin division */
    else
    freq_hz = base_clk / (2U * divisor);

    xil_printf("CLK_CTRL:    0x%08X\r\n", (unsigned int)clk_ctrl);
    xil_printf("Base clock:  %d Hz (%d MHz)\r\n",
               (int)base_clk, (int)(base_clk / 1000000));
    xil_printf("Divisor:     %d\r\n", (int)divisor);
    xil_printf("SD Clock:    %d Hz (%d MHz)\r\n",
               (int)freq_hz, (int)(freq_hz / 1000000));
    xil_printf("BusWidth:    %d bits\r\n",
               (int)(SdInstance.BusWidth == 2 ? 4 : 1));
    xil_printf("CardType:    %s\r\n",
               SdInstance.CardType == XSDPS_CARD_SD ? "SD" : "otros");

    /* Confirmacion con osciloscopio */
    xil_printf("Verificado:  50 MHz (osciloscopio en pin CLK)\r\n");
}

/* ─── Escribir .txt ────────────────────────────────── */
void test_write_txt(void)
{
    UINT bytes_written;

    xil_printf("\r\n-- Escribiendo hola.txt --\r\n");

    fr = f_open(&fil, "0:hola.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_open escritura (%d)\r\n", fr);
        return;
    }

    snprintf(write_buf, BUF_SIZE,
             "Hola desde la Zybo Z7-20!\r\n"
             "Prueba SDIO con polling\r\n"
             "Si lees esto, funciono correctamente\r\n");

    fr = f_write(&fil, write_buf, strlen(write_buf), &bytes_written);
    if (fr != FR_OK || bytes_written != strlen(write_buf))
        xil_printf("ERROR: f_write (%d)\r\n", fr);
    else
        xil_printf("OK: %u bytes escritos\r\n", bytes_written);

    f_close(&fil);
}

/* ─── Leer .txt ────────────────────────────────────── */
void test_read_txt(void)
{
    UINT bytes_read;

    xil_printf("\r\n-- Leyendo hola.txt --\r\n");

    fr = f_open(&fil, "0:hola.txt", FA_READ);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_open lectura (%d)\r\n", fr);
        return;
    }

    memset(read_buf, 0, BUF_SIZE);
    fr = f_read(&fil, read_buf, BUF_SIZE - 1, &bytes_read);
    if (fr != FR_OK)
        xil_printf("ERROR: f_read (%d)\r\n", fr);
    else {
        xil_printf("OK: %u bytes leidos\r\n", bytes_read);
        xil_printf("Contenido:\r\n%s\r\n", read_buf);
    }

    f_close(&fil);
}

/* ─── Escribir .bin ────────────────────────────────── */
void test_write_bin(void)
{
    UINT bytes_written;
    u8 bin_buf[256];
    u32 i;

    xil_printf("\r\n-- Escribiendo datos.bin --\r\n");

    for (i = 0; i < 256; i++)
        bin_buf[i] = (u8)i;

    fr = f_open(&fil, "0:datos.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_open bin (%d)\r\n", fr);
        return;
    }

    fr = f_write(&fil, bin_buf, 256, &bytes_written);
    if (fr != FR_OK || bytes_written != 256)
        xil_printf("ERROR: f_write bin (%d)\r\n", fr);
    else
        xil_printf("OK: %u bytes escritos (patron 0x00-0xFF)\r\n", bytes_written);

    f_close(&fil);
}