#include <stdio.h>
#include <string.h>
#include "xparameters.h"
#include "xsdps.h"
#include "xil_printf.h"
#include "ff.h"

/* ─── Objetos globales ─────────────────────────────── */
static FATFS fatfs;          /* Sistema de archivos montado   */
static FIL   fil;            /* Descriptor de archivo         */
static FRESULT fr;           /* Resultado de operaciones FatFs*/

/* ─── Buffers ──────────────────────────────────────── */
#define BUF_SIZE 512
static char write_buf[BUF_SIZE];
static char read_buf[BUF_SIZE];

/* ─── Prototipos ───────────────────────────────────── */
void test_write_txt(void);
void test_read_txt(void);
void test_write_bin(void);

/* ════════════════════════════════════════════════════ */
int main(void)
{
    xil_printf("\r\n=== SDIO Polling Test ===\r\n");

    /* 1. Montar sistema de archivos */
    fr = f_mount(&fatfs, "0:", 1);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_mount fallo (%d)\r\n", fr);
        return -1;
    }
    xil_printf("SD montada correctamente\r\n");

    /* 2. Pruebas */
    test_write_txt();
    test_read_txt();
    test_write_bin();

    /* 3. Desmontar */
    f_unmount("0:");
    xil_printf("\r\n=== Test finalizado ===\r\n");

    return 0;
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
    if (fr != FR_OK || bytes_written != strlen(write_buf)) {
        xil_printf("ERROR: f_write (%d)\r\n", fr);
    } else {
        xil_printf("OK: %u bytes escritos\r\n", bytes_written);
    }

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
    if (fr != FR_OK) {
        xil_printf("ERROR: f_read (%d)\r\n", fr);
    } else {
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

    /* Patron 0x00 a 0xFF */
    for (i = 0; i < 256; i++) {
        bin_buf[i] = (u8)i;
    }

    fr = f_open(&fil, "0:datos.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        xil_printf("ERROR: f_open bin (%d)\r\n", fr);
        return;
    }

    fr = f_write(&fil, bin_buf, 256, &bytes_written);
    if (fr != FR_OK || bytes_written != 256) {
        xil_printf("ERROR: f_write bin (%d)\r\n", fr);
    } else {
        xil_printf("OK: %u bytes escritos (patron 0x00-0xFF)\r\n", bytes_written);
    }

    f_close(&fil);
}