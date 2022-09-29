//
// Created by TD on 2021/9/22.
//
#include "imgui_warp.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::dear {

struct input_text_callback_userdata {
  std::string Str;
  FSys::path *Path;
  ImGuiInputTextCallback ChainCallback;
  void *ChainCallbackUserData;
};
static int InputTextCallback(ImGuiInputTextCallbackData *data) {
  input_text_callback_userdata *user_data = (input_text_callback_userdata *)data->UserData;
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    // Resize string callback
    // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
    std::string &str = user_data->Str;
    IM_ASSERT(data->Buf == str.c_str());
    str.resize(data->BufTextLen);
    data->Buf          = (char *)str.c_str();
    *(user_data->Path) = FSys::path{user_data->Str};
  } else if (user_data->ChainCallback) {
    // Forward to user callback, if any
    data->UserData     = user_data->ChainCallbackUserData;
    *(user_data->Path) = FSys::path{user_data->Str};
    return user_data->ChainCallback(data);
  }
  return 0;
}

// bool InputText(const char *label,
//                FSys::path *in_path,
//                ImGuiInputTextFlags flags,
//                ImGuiInputTextCallback callback,
//                void *user_data) {
//   IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
//   input_text_callback_userdata cb_user_data;
//   cb_user_data.Path                  = in_path;
//   cb_user_data.Str                   = in_path->generic_string();
//   cb_user_data.ChainCallback         = callback;
//   cb_user_data.ChainCallbackUserData = user_data;
//   return ImGui::InputText(label,
//                           (char *)cb_user_data.Str.c_str(),
//                           cb_user_data.Str.capacity() + 1,
//                           flags,
//                           InputTextCallback,
//                           (void *)&cb_user_data);
// }
}  // namespace doodle::dear
