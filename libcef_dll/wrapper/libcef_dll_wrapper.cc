// Copyright (c) 2022 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//
// $hash=f1ec7f73b35927e943a058141d73d449dd31e43c$
//

#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_crash_util_capi.h"
#include "include/capi/cef_file_util_capi.h"
#include "include/capi/cef_i18n_util_capi.h"
#include "include/capi/cef_origin_whitelist_capi.h"
#include "include/capi/cef_parser_capi.h"
#include "include/capi/cef_path_util_capi.h"
#include "include/capi/cef_process_util_capi.h"
#include "include/capi/cef_scheme_capi.h"
#include "include/capi/cef_ssl_info_capi.h"
#include "include/capi/cef_task_capi.h"
#include "include/capi/cef_trace_capi.h"
#include "include/capi/cef_v8_capi.h"
#include "include/capi/test/cef_test_helpers_capi.h"
#include "include/cef_api_hash.h"
#include "include/cef_app.h"
#include "include/cef_crash_util.h"
#include "include/cef_file_util.h"
#include "include/cef_i18n_util.h"
#include "include/cef_origin_whitelist.h"
#include "include/cef_parser.h"
#include "include/cef_path_util.h"
#include "include/cef_process_util.h"
#include "include/cef_scheme.h"
#include "include/cef_ssl_info.h"
#include "include/cef_task.h"
#include "include/cef_trace.h"
#include "include/cef_v8.h"
#include "include/test/cef_test_helpers.h"
#include "libcef_dll/cpptoc/app_cpptoc.h"
#include "libcef_dll/cpptoc/completion_callback_cpptoc.h"
#include "libcef_dll/cpptoc/end_tracing_callback_cpptoc.h"
#include "libcef_dll/cpptoc/scheme_handler_factory_cpptoc.h"
#include "libcef_dll/cpptoc/task_cpptoc.h"
#include "libcef_dll/cpptoc/v8handler_cpptoc.h"
#include "libcef_dll/ctocpp/binary_value_ctocpp.h"
#include "libcef_dll/ctocpp/command_line_ctocpp.h"
#include "libcef_dll/ctocpp/frame_ctocpp.h"
#include "libcef_dll/ctocpp/value_ctocpp.h"
#include "libcef_dll/shutdown_checker.h"
#include "libcef_dll/transfer_util.h"

// Define used to facilitate parsing.
#define CEF_GLOBAL

// GLOBAL METHODS - Body may be edited by hand.

NO_SANITIZE("cfi-icall")
CEF_GLOBAL int CefExecuteProcess(const CefMainArgs& args,
                                 CefRefPtr<CefApp> application,
                                 void* windows_sandbox_info) {
  const char* api_hash = cef_api_hash(0);
  if (strcmp(api_hash, CEF_API_HASH_PLATFORM)) {
    // The libcef API hash does not match the current header API hash.
    NOTREACHED();
    return 0;
  }

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: application, windows_sandbox_info

  // Execute
  int _retval = cef_execute_process(&args, CefAppCppToC::Wrap(application),
                                    windows_sandbox_info);

  // Return type: simple
  return _retval;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefInitialize(const CefMainArgs& args,
                              const CefSettings& settings,
                              CefRefPtr<CefApp> application,
                              void* windows_sandbox_info) {
  const char* api_hash = cef_api_hash(0);
  if (strcmp(api_hash, CEF_API_HASH_PLATFORM)) {
    // The libcef API hash does not match the current header API hash.
    NOTREACHED();
    return false;
  }

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: application, windows_sandbox_info

  // Execute
  int _retval = cef_initialize(
      &args, &settings, CefAppCppToC::Wrap(application), windows_sandbox_info);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefShutdown() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

#if DCHECK_IS_ON()
  shutdown_checker::SetIsShutdown();
#endif

  // Execute
  cef_shutdown();
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefDoMessageLoopWork() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  cef_do_message_loop_work();
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefRunMessageLoop() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  cef_run_message_loop();
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefQuitMessageLoop() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  cef_quit_message_loop();
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefSetOSModalLoop(bool osModalLoop) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  cef_set_osmodal_loop(osModalLoop);
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL void CefEnableHighDPISupport() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  cef_enable_highdpi_support();
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL bool CefCrashReportingEnabled() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_crash_reporting_enabled();

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL
void CefSetCrashKeyValue(const CefString& key, const CefString& value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: key; type: string_byref_const
  DCHECK(!key.empty());
  if (key.empty())
    return;
  // Unverified params: value

  // Execute
  cef_set_crash_key_value(key.GetStruct(), value.GetStruct());
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefCreateDirectory(const CefString& full_path) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: full_path; type: string_byref_const
  DCHECK(!full_path.empty());
  if (full_path.empty())
    return false;

  // Execute
  int _retval = cef_create_directory(full_path.GetStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefGetTempDirectory(CefString& temp_dir) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_get_temp_directory(temp_dir.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefCreateNewTempDirectory(const CefString& prefix,
                                          CefString& new_temp_path) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: prefix

  // Execute
  int _retval = cef_create_new_temp_directory(
      prefix.GetStruct(), new_temp_path.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefCreateTempDirectoryInDirectory(const CefString& base_dir,
                                                  const CefString& prefix,
                                                  CefString& new_dir) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: base_dir; type: string_byref_const
  DCHECK(!base_dir.empty());
  if (base_dir.empty())
    return false;
  // Unverified params: prefix

  // Execute
  int _retval = cef_create_temp_directory_in_directory(
      base_dir.GetStruct(), prefix.GetStruct(), new_dir.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefDirectoryExists(const CefString& path) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: path; type: string_byref_const
  DCHECK(!path.empty());
  if (path.empty())
    return false;

  // Execute
  int _retval = cef_directory_exists(path.GetStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefDeleteFile(const CefString& path, bool recursive) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: path; type: string_byref_const
  DCHECK(!path.empty());
  if (path.empty())
    return false;

  // Execute
  int _retval = cef_delete_file(path.GetStruct(), recursive);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefZipDirectory(const CefString& src_dir,
                                const CefString& dest_file,
                                bool include_hidden_files) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: src_dir; type: string_byref_const
  DCHECK(!src_dir.empty());
  if (src_dir.empty())
    return false;
  // Verify param: dest_file; type: string_byref_const
  DCHECK(!dest_file.empty());
  if (dest_file.empty())
    return false;

  // Execute
  int _retval = cef_zip_directory(src_dir.GetStruct(), dest_file.GetStruct(),
                                  include_hidden_files);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL void CefLoadCRLSetsFile(const CefString& path) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: path; type: string_byref_const
  DCHECK(!path.empty());
  if (path.empty())
    return;

  // Execute
  cef_load_crlsets_file(path.GetStruct());
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL bool CefIsRTL() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_is_rtl();

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL
bool CefAddCrossOriginWhitelistEntry(const CefString& source_origin,
                                     const CefString& target_protocol,
                                     const CefString& target_domain,
                                     bool allow_target_subdomains) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: source_origin; type: string_byref_const
  DCHECK(!source_origin.empty());
  if (source_origin.empty())
    return false;
  // Verify param: target_protocol; type: string_byref_const
  DCHECK(!target_protocol.empty());
  if (target_protocol.empty())
    return false;
  // Unverified params: target_domain

  // Execute
  int _retval = cef_add_cross_origin_whitelist_entry(
      source_origin.GetStruct(), target_protocol.GetStruct(),
      target_domain.GetStruct(), allow_target_subdomains);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL
bool CefRemoveCrossOriginWhitelistEntry(const CefString& source_origin,
                                        const CefString& target_protocol,
                                        const CefString& target_domain,
                                        bool allow_target_subdomains) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: source_origin; type: string_byref_const
  DCHECK(!source_origin.empty());
  if (source_origin.empty())
    return false;
  // Verify param: target_protocol; type: string_byref_const
  DCHECK(!target_protocol.empty());
  if (target_protocol.empty())
    return false;
  // Unverified params: target_domain

  // Execute
  int _retval = cef_remove_cross_origin_whitelist_entry(
      source_origin.GetStruct(), target_protocol.GetStruct(),
      target_domain.GetStruct(), allow_target_subdomains);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL bool CefClearCrossOriginWhitelist() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_clear_cross_origin_whitelist();

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefResolveURL(const CefString& base_url,
                              const CefString& relative_url,
                              CefString& resolved_url) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: base_url; type: string_byref_const
  DCHECK(!base_url.empty());
  if (base_url.empty())
    return false;
  // Verify param: relative_url; type: string_byref_const
  DCHECK(!relative_url.empty());
  if (relative_url.empty())
    return false;

  // Execute
  int _retval = cef_resolve_url(base_url.GetStruct(), relative_url.GetStruct(),
                                resolved_url.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefParseURL(const CefString& url, CefURLParts& parts) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: url; type: string_byref_const
  DCHECK(!url.empty());
  if (url.empty())
    return false;

  // Execute
  int _retval = cef_parse_url(url.GetStruct(), &parts);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefCreateURL(const CefURLParts& parts, CefString& url) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_create_url(&parts, url.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString
CefFormatUrlForSecurityDisplay(const CefString& origin_url) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: origin_url; type: string_byref_const
  DCHECK(!origin_url.empty());
  if (origin_url.empty())
    return CefString();

  // Execute
  cef_string_userfree_t _retval =
      cef_format_url_for_security_display(origin_url.GetStruct());

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString CefGetMimeType(const CefString& extension) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: extension; type: string_byref_const
  DCHECK(!extension.empty());
  if (extension.empty())
    return CefString();

  // Execute
  cef_string_userfree_t _retval = cef_get_mime_type(extension.GetStruct());

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL
void CefGetExtensionsForMimeType(const CefString& mime_type,
                                 std::vector<CefString>& extensions) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: mime_type; type: string_byref_const
  DCHECK(!mime_type.empty());
  if (mime_type.empty())
    return;

  // Translate param: extensions; type: string_vec_byref
  cef_string_list_t extensionsList = cef_string_list_alloc();
  DCHECK(extensionsList);
  if (extensionsList)
    transfer_string_list_contents(extensions, extensionsList);

  // Execute
  cef_get_extensions_for_mime_type(mime_type.GetStruct(), extensionsList);

  // Restore param:extensions; type: string_vec_byref
  if (extensionsList) {
    extensions.clear();
    transfer_string_list_contents(extensionsList, extensions);
    cef_string_list_free(extensionsList);
  }
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString CefBase64Encode(const void* data, size_t data_size) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: data; type: simple_byaddr
  DCHECK(data);
  if (!data)
    return CefString();

  // Execute
  cef_string_userfree_t _retval = cef_base64encode(data, data_size);

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefRefPtr<CefBinaryValue> CefBase64Decode(const CefString& data) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: data; type: string_byref_const
  DCHECK(!data.empty());
  if (data.empty())
    return nullptr;

  // Execute
  cef_binary_value_t* _retval = cef_base64decode(data.GetStruct());

  // Return type: refptr_same
  return CefBinaryValueCToCpp::Wrap(_retval);
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString CefURIEncode(const CefString& text, bool use_plus) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: text; type: string_byref_const
  DCHECK(!text.empty());
  if (text.empty())
    return CefString();

  // Execute
  cef_string_userfree_t _retval = cef_uriencode(text.GetStruct(), use_plus);

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString CefURIDecode(const CefString& text,
                                  bool convert_to_utf8,
                                  cef_uri_unescape_rule_t unescape_rule) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: text; type: string_byref_const
  DCHECK(!text.empty());
  if (text.empty())
    return CefString();

  // Execute
  cef_string_userfree_t _retval =
      cef_uridecode(text.GetStruct(), convert_to_utf8, unescape_rule);

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefRefPtr<CefValue> CefParseJSON(const CefString& json_string,
                                            cef_json_parser_options_t options) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: json_string; type: string_byref_const
  DCHECK(!json_string.empty());
  if (json_string.empty())
    return nullptr;

  // Execute
  cef_value_t* _retval = cef_parse_json(json_string.GetStruct(), options);

  // Return type: refptr_same
  return CefValueCToCpp::Wrap(_retval);
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefRefPtr<CefValue> CefParseJSON(const void* json,
                                            size_t json_size,
                                            cef_json_parser_options_t options) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: json; type: simple_byaddr
  DCHECK(json);
  if (!json)
    return nullptr;

  // Execute
  cef_value_t* _retval = cef_parse_json_buffer(json, json_size, options);

  // Return type: refptr_same
  return CefValueCToCpp::Wrap(_retval);
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefRefPtr<CefValue> CefParseJSONAndReturnError(
    const CefString& json_string,
    cef_json_parser_options_t options,
    CefString& error_msg_out) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: json_string; type: string_byref_const
  DCHECK(!json_string.empty());
  if (json_string.empty())
    return nullptr;

  // Execute
  cef_value_t* _retval = cef_parse_jsonand_return_error(
      json_string.GetStruct(), options, error_msg_out.GetWritableStruct());

  // Return type: refptr_same
  return CefValueCToCpp::Wrap(_retval);
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL CefString CefWriteJSON(CefRefPtr<CefValue> node,
                                  cef_json_writer_options_t options) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: node; type: refptr_same
  DCHECK(node.get());
  if (!node.get())
    return CefString();

  // Execute
  cef_string_userfree_t _retval =
      cef_write_json(CefValueCToCpp::Unwrap(node), options);

  // Return type: string
  CefString _retvalStr;
  _retvalStr.AttachToUserFree(_retval);
  return _retvalStr;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefGetPath(PathKey key, CefString& path) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_get_path(key, path.GetWritableStruct());

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefLaunchProcess(CefRefPtr<CefCommandLine> command_line) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: command_line; type: refptr_same
  DCHECK(command_line.get());
  if (!command_line.get())
    return false;

  // Execute
  int _retval = cef_launch_process(CefCommandLineCToCpp::Unwrap(command_line));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefRegisterSchemeHandlerFactory(
    const CefString& scheme_name,
    const CefString& domain_name,
    CefRefPtr<CefSchemeHandlerFactory> factory) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: scheme_name; type: string_byref_const
  DCHECK(!scheme_name.empty());
  if (scheme_name.empty())
    return false;
  // Unverified params: domain_name, factory

  // Execute
  int _retval = cef_register_scheme_handler_factory(
      scheme_name.GetStruct(), domain_name.GetStruct(),
      CefSchemeHandlerFactoryCppToC::Wrap(factory));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL bool CefClearSchemeHandlerFactories() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_clear_scheme_handler_factories();

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefIsCertStatusError(cef_cert_status_t status) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_is_cert_status_error(status);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL bool CefCurrentlyOn(CefThreadId threadId) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = cef_currently_on(threadId);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefPostTask(CefThreadId threadId, CefRefPtr<CefTask> task) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: task; type: refptr_diff
  DCHECK(task.get());
  if (!task.get())
    return false;

  // Execute
  int _retval = cef_post_task(threadId, CefTaskCppToC::Wrap(task));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefPostDelayedTask(CefThreadId threadId,
                                   CefRefPtr<CefTask> task,
                                   int64 delay_ms) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: task; type: refptr_diff
  DCHECK(task.get());
  if (!task.get())
    return false;

  // Execute
  int _retval =
      cef_post_delayed_task(threadId, CefTaskCppToC::Wrap(task), delay_ms);

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefBeginTracing(const CefString& categories,
                                CefRefPtr<CefCompletionCallback> callback) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: categories, callback

  // Execute
  int _retval = cef_begin_tracing(categories.GetStruct(),
                                  CefCompletionCallbackCppToC::Wrap(callback));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefEndTracing(const CefString& tracing_file,
                              CefRefPtr<CefEndTracingCallback> callback) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: tracing_file, callback

  // Execute
  int _retval = cef_end_tracing(tracing_file.GetStruct(),
                                CefEndTracingCallbackCppToC::Wrap(callback));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall") CEF_GLOBAL int64 CefNowFromSystemTraceTime() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int64 _retval = cef_now_from_system_trace_time();

  // Return type: simple
  return _retval;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL bool CefRegisterExtension(const CefString& extension_name,
                                     const CefString& javascript_code,
                                     CefRefPtr<CefV8Handler> handler) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: extension_name; type: string_byref_const
  DCHECK(!extension_name.empty());
  if (extension_name.empty())
    return false;
  // Verify param: javascript_code; type: string_byref_const
  DCHECK(!javascript_code.empty());
  if (javascript_code.empty())
    return false;
  // Unverified params: handler

  // Execute
  int _retval = cef_register_extension(extension_name.GetStruct(),
                                       javascript_code.GetStruct(),
                                       CefV8HandlerCppToC::Wrap(handler));

  // Return type: bool
  return _retval ? true : false;
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL void CefExecuteJavaScriptWithUserGestureForTests(
    CefRefPtr<CefFrame> frame,
    const CefString& javascript) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: frame; type: refptr_same
  DCHECK(frame.get());
  if (!frame.get())
    return;
  // Unverified params: javascript

  // Execute
  cef_execute_java_script_with_user_gesture_for_tests(
      CefFrameCToCpp::Unwrap(frame), javascript.GetStruct());
}

NO_SANITIZE("cfi-icall")
CEF_GLOBAL void CefSetDataDirectoryForTests(const CefString& dir) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: dir; type: string_byref_const
  DCHECK(!dir.empty());
  if (dir.empty())
    return;

  // Execute
  cef_set_data_directory_for_tests(dir.GetStruct());
}
