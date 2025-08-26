/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file    ml-lxm-service-internal.h
 * @date    23 JULY 2025
 * @brief   Machine Learning LXM(LLM, LVM, etc.) Service API
 * @see     https://github.com/nnstreamer/api
 * @author  Hyunil Park <hyunil46.park@samsung.com>
 * @bug     No known bugs except for NYI items
 */

/**
 * @example  sample_lxm_service.c
 * @brief    Sample application demonstrating ML LXM Service API usage
 *
 * This sample shows how to:
 * - Create and configure an LXM session
 * - Build prompts with text and instructions
 * - Generate streaming responses with custom options
 * - Handle token callbacks for real-time processing
 *
 * Configuration file example (config.json):
 * @code
 * {
 *     "single" :
 *     {
 *         "framework" : "flare",
 *         "model" : ["sflare_if_4bit_3b.bin"],
 *         "adapter" : ["history_lora.bin"],
 *         "custom" : "tokenizer_path:tokenizer.json,backend:CPU,output_size:1024,model_type:3B,data_type:W4A32",
 *         "invoke_dynamic" : "true",
 *     }
 * }
 * @endcode
 *
 * Basic usage workflow:
 * @code
 * // 1. Create session with callback
 * ml_lxm_session_h session;
 * ml_lxm_session_create("/path/to/config.json", NULL, token_handler, NULL, &session);
 *
 * // 2. Create prompt
 * ml_lxm_prompt_h prompt;
 * ml_lxm_prompt_create(&prompt);
 * ml_lxm_prompt_append_text(prompt, "Hello AI");
 *
 * // 3. Generate response with options (callback is already set during session creation)
 * ml_option_h options = NULL;
 * ml_option_create(&options);
 * ml_option_set(options, "temperature", g_strdup_printf("%f", 1.0), g_free);
 * ml_option_set(options, "max_tokens", g_strdup_printf("%zu", (size_t)50), g_free);
 * ml_lxm_session_respond(session, prompt, options);
 * ml_option_destroy(options);
 *
 * // 4. Cleanup
 * ml_lxm_prompt_destroy(prompt);
 * ml_lxm_session_destroy(session);
 * @endcode
 *
 * Complete example with token callback:
 * @code
 * #include "ml-lxm-service-internal.h"
 * #include <iostream>
 *
 * static void token_handler(ml_service_event_e event,
 *                          ml_information_h event_data,
 *                          void *user_data);
 *
 * int main() {
 *   ml_lxm_session_h session = NULL;
 *   ml_lxm_prompt_h prompt = NULL;
 *   int ret;
 *
 *   // Check availability first
 *   ml_lxm_availability_e status;
 *   ret = ml_lxm_check_availability(&status);
 *   if (ret != ML_ERROR_NONE || status != ML_LXM_AVAILABILITY_AVAILABLE) {
 *     std::cout << "LXM service not available" << std::endl;
 *     return -1;
 *   }
 *
 *   // 1. Create session with config, instructions, and callback
 *   ret = ml_lxm_session_create("/path/to/config.json", "You are a helpful AI assistant", token_handler, NULL, &session);
 *   if (ret != ML_ERROR_NONE) {
 *     std::cout << "Failed to create session" << std::endl;
 *     return -1;
 *   }
 *
 *   // 2. Create prompt
 *   ret = ml_lxm_prompt_create(&prompt);
 *   if (ret != ML_ERROR_NONE) {
 *     std::cout << "Failed to create prompt" << std::endl;
 *     ml_lxm_session_destroy(session);
 *     return -1;
 *   }
 *
 *   // Add text to prompt
 *   ret = ml_lxm_prompt_append_text(prompt, "Explain quantum computing in simple terms");
 *   if (ret != ML_ERROR_NONE) {
 *     std::cout << "Failed to append text to prompt" << std::endl;
 *     ml_lxm_prompt_destroy(prompt);
 *     ml_lxm_session_destroy(session);
 *     return -1;
 *   }
 *
 *   // 3. Generate response with custom options
 *   ml_option_h options = NULL;
 *   ml_option_create(&options);
 *   ml_option_set(options, "temperature", g_strdup_printf("%f", 1.2), g_free);
 *   ml_option_set(options, "max_tokens", g_strdup_printf("%zu", (size_t)128), g_free);
 *
 *   std::cout << "AI Response: ";
 *   ret = ml_lxm_session_respond(session, prompt, options);
 *   ml_option_destroy(options);
 *   if (ret != ML_ERROR_NONE) {
 *     std::cout << "Failed to generate response" << std::endl;
 *   }
 *   std::cout << std::endl;
 *
 *   // 4. Cleanup
 *   ml_lxm_prompt_destroy(prompt);
 *   ml_lxm_session_destroy(session);
 *
 *   return 0;
 * }
 *
 * static void token_handler(ml_service_event_e event,
 *                          ml_information_h event_data,
 *                          void *user_data) {
 *   ml_tensors_data_h data = NULL;
 *   void *_raw = NULL;
 *   size_t _size = 0;
 *   int ret;
 *
 *   switch (event) {
 *   case ML_SERVICE_EVENT_NEW_DATA:
 *     if (event_data != NULL) {
 *       ret = ml_information_get(event_data, "data", &data);
 *       if (ret == ML_ERROR_NONE) {
 *         ret = ml_tensors_data_get_tensor_data(data, 0U, &_raw, &_size);
 *         if (ret == ML_ERROR_NONE && _raw != NULL && _size > 0) {
 *           std::cout.write(static_cast<const char *>(_raw), _size);
 *           std::cout.flush();
 *         }
 *       }
 *     }
 *     break;
 *   default:
 *     break;
 *   }
 * }
 * @endcode
 */

#ifndef __ML_LXM_SERVICE_INTERNAL_H__
#define __ML_LXM_SERVICE_INTERNAL_H__

#include <stdlib.h>
#include <ml-api-service.h>
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Enumeration for LXM service availability status.
 */
typedef enum
{
  ML_LXM_AVAILABILITY_AVAILABLE = 0,
  ML_LXM_AVAILABILITY_DEVICE_NOT_ELIGIBLE,
  ML_LXM_AVAILABILITY_SERVICE_DISABLED,
  ML_LXM_AVAILABILITY_MODEL_NOT_READY,
  ML_LXM_AVAILABILITY_UNKNOWN
} ml_lxm_availability_e;

/**
 * @brief Checks LXM service availability.
 * @param[out] status Current availability status.
 * @return ML_ERROR_NONE on success, error code otherwise.
 */
int ml_lxm_check_availability (ml_lxm_availability_e * status);

/**
 * @brief A handle for lxm session.
 */
typedef void *ml_lxm_session_h;

/**
 * @brief Creates an LXM session with mandatory callback.
 * @param[in] config_path Path to configuration file.
 * @param[in] instructions Initial instructions (optional).
 * @param[in] callback Callback function for session events (mandatory).
 * @param[in] user_data User data to be passed to the callback.
 * @param[out] session Session handle.
 * @return ML_ERROR_NONE on success.
 * @note The callback parameter is mandatory and will be set during session creation.
 */
int ml_lxm_session_create (const char *config_path, const char *instructions, ml_service_event_cb callback, void *user_data, ml_lxm_session_h * session);

/**
 * @brief Destroys an LXM session.
 * @param[in] session Session handle.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_session_destroy (ml_lxm_session_h session);

/**
 * @brief A handle for lxm prompt.
 */
typedef void *ml_lxm_prompt_h;

/**
 * @brief Creates a prompt object.
 * @param[out] prompt Prompt handle.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_prompt_create (ml_lxm_prompt_h * prompt);

/**
 * @brief Appends text to a prompt.
 * @param[in] prompt Prompt handle.
 * @param[in] text Text to append.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_prompt_append_text (ml_lxm_prompt_h prompt, const char *text);

/**
 * @brief Appends an instruction to a prompt.
 * @param[in] prompt Prompt handle.
 * @param[in] instruction Instruction to append.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_prompt_append_instruction (ml_lxm_prompt_h prompt, const char *instruction);

/**
 * @brief Destroys a prompt object.
 * @param[in] prompt Prompt handle.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_prompt_destroy (ml_lxm_prompt_h prompt);

/**
 * @brief Sets runtime instructions for a session.
 * @param[in] session Session handle.
 * @param[in] instructions New instructions.
 * @return ML_ERROR_NONE on success.
 */
int ml_lxm_session_set_instructions (ml_lxm_session_h session, const char *instructions);


/**
 * @brief Generates an token-streamed response.
 * @param[in] session Session handle.
 * @param[in] prompt Prompt handle.
 * @param[in] options Generation parameters (ml_option_h).
 * @return ML_ERROR_NONE on success.
 * @note The callback should be set using ml_lxm_session_set_event_cb() before calling this function.
 */
int ml_lxm_session_respond (ml_lxm_session_h session, ml_lxm_prompt_h prompt, ml_option_h options);

#ifdef __cplusplus
}
#endif
#endif
/* __ML_LXM_SERVICE_INTERNAL_H__ */
