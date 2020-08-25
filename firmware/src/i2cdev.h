/**
 * @file i2cdev.h
 * @defgroup i2cdev i2cdev
 * @{
 *
 * ESP-IDF I2C master thread-safe functions for communication with I2C slave
 *
 * Copyright (C) 2018 Ruslan V. Uss <https://github.com/UncleRus>
 * Copyright (C) 2020 Anton Semjonov
 * 
 * 2020-08: Modified to remove semaphores due to "xQueueGenericReceive" errors.
 *
 * MIT Licensed as described in the file LICENSE in
 * https://github.com/UncleRus/esp-idf-lib/blob/master/components/i2cdev/LICENSE
 */
#ifndef __I2CDEV_H__
#define __I2CDEV_H__

#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_I2CDEV_TIMEOUT 1000

/**
 * I2C device descriptor
 */
typedef struct
{
    i2c_port_t port;         //!< I2C port number, 0 or 1
    i2c_config_t cfg;        //!< I2C driver configuration
    uint8_t addr;            //!< Unshifted address
    SemaphoreHandle_t mutex; //!< Device mutex
} i2c_dev_t;

/**
 * @brief Read from slave device
 *
 * Issue a send operation of \p out_data register adress, followed by reading \p in_size bytes
 * from slave into \p in_data .
 * Function is thread-safe.
 * @param[in] dev Device descriptor
 * @param[in] out_data Pointer to data to send if non-null
 * @param[in] out_size Size of data to send
 * @param[out] in_data Pointer to input data buffer
 * @param[in] in_size Number of byte to read
 * @return ESP_OK on success
 */
esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data,
        size_t out_size, void *in_data, size_t in_size);

/**
 * @brief Write to slave device
 *
 * Write \p out_size bytes from \p out_data to slave into \p out_reg register address.
 * Function is thread-safe.
 * @param[in] dev Device descriptor
 * @param[in] out_reg Pointer to register address to send if non-null
 * @param[in] out_reg_size Size of register address
 * @param[in] out_data Pointer to data to send
 * @param[in] out_size Size of data to send
 * @return ESP_OK on success
 */
esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg,
        size_t out_reg_size, const void *out_data, size_t out_size);

/**
 * @brief Read from register with an 8-bit address
 *
 * Shortcut to i2c_dev_read().
 * @param[in] dev Device descriptor
 * @param[in] reg Register address
 * @param[out] in_data Pointer to input data buffer
 * @param[in] in_size Number of byte to read
 * @return ESP_OK on success
 */
esp_err_t i2c_dev_read_reg(const i2c_dev_t *dev, uint8_t reg,
        void *in_data, size_t in_size);

/**
 * @brief Write to register with an 8-bit address
 *
 * Shortcut to i2c_dev_write().
 * @param[in] dev Device descriptor
 * @param[in] reg Register address
 * @param[in] out_data Pointer to data to send
 * @param[in] out_size Size of data to send
 * @return ESP_OK on success
 */
esp_err_t i2c_dev_write_reg(const i2c_dev_t *dev, uint8_t reg,
        const void *out_data, size_t out_size);

#define I2C_DEV_CHECK(dev,X) do { \
        esp_err_t ___ = X; \
        if (___ != ESP_OK) { \
            return ___; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __I2CDEV_H__ */
