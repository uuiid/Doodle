//
// Created by zhanghang on 2023/10/23.
//

#include "zh_model_upload.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <long_task/image_to_move.h>
#include <utility>

#include "boost/filesystem/path.hpp"
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/locale.hpp>

namespace doodle::gui {

enum MyItemColumnID
{
    MyItemColumnID_ID,
    MyItemColumnID_Name,
    MyItemColumnID_Action,
    MyItemColumnID_Quantity,
    MyItemColumnID_Description
};
//-------------------------------------
struct MyItem
{
    int         ID;
    const char* Name;
    int         Quantity;
    //---------------------------
    static const ImGuiTableSortSpecs* s_current_sort_specs;
    //------------------------------
    static void SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, MyItem* items, int items_count)
    {
        s_current_sort_specs = sort_specs; // Store in variable accessible by the sort function.
        if (items_count > 1)
            qsort(items, (size_t)items_count, sizeof(items[0]), MyItem::CompareWithSortSpecs);
        s_current_sort_specs = NULL;
    }
    // Compare function to be used by qsort()
    static int IMGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const MyItem* a = (const MyItem*)lhs;
        const MyItem* b = (const MyItem*)rhs;
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            int delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case MyItemColumnID_ID:             delta = (a->ID - b->ID);                break;
            case MyItemColumnID_Name:           delta = (strcmp(a->Name, b->Name));     break;
            case MyItemColumnID_Quantity:       delta = (a->Quantity - b->Quantity);    break;
            case MyItemColumnID_Description:    delta = (strcmp(a->Name, b->Name));     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
        }

        // qsort() is instable so always return a way to differenciate items.
        // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
        return (a->ID - b->ID);
    }
};
const ImGuiTableSortSpecs* MyItem::s_current_sort_specs = NULL;

class zh_model_upload::image_arg : public gui::gui_cache<std::string> {
 public:
  using base_type = gui::gui_cache<std::string>;
  explicit image_arg(
      const entt::handle& in_handle, std::vector<FSys::path> in_image_attr, const std::string& in_show_str
  )
      : base_type(in_show_str), out_handle(in_handle), image_attr(std::move(in_image_attr)){};

  entt::handle out_handle;
  std::vector<FSys::path> image_attr;
};

class zh_model_upload::impl {
 public:
  gui::gui_cache<std::string> out_path{"输出路径"s, ""s};

  struct gui_path : gui_cache_path {
    FSys::path stem{};
  };
  gui::gui_cache<std::string, gui_path> export_path{"导出路径"s, ""s};

  using image_cache = zh_model_upload::image_arg;
  using video_cache = gui::gui_cache<std::string>;

  std::vector<image_cache> image_to_video_list;
  std::vector<video_cache> video_list;
  entt::handle out_video_h;
  std::string title_name_;
  bool open{true};
};

zh_model_upload::zh_model_upload() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  p_i->out_video_h = entt::handle{*g_reg(), g_reg()->create()};
}

bool zh_model_upload::render() {
  if (ImGui::InputText(*p_i->out_path.gui_name, &p_i->out_path.data)) {
    ::ranges::for_each(p_i->image_to_video_list, [this](impl::image_cache& in_image_cache) {
      in_image_cache.out_handle.emplace_or_replace<FSys::path>(p_i->out_path.data);
    });
  };
  ImGui::SameLine();
  if (ImGui::Button("选择")) {
    auto l_ptr = std::make_shared<FSys::path>();
    //    boost::asio::post(
    //        make_process_adapter<file_dialog>(
    //            strand_gui{g_io_context()},
    //            file_dialog::dialog_args{l_ptr}
    //                .set_title("选择目录"s)
    //                .set_use_dir()
    //        )
    //            .next([this, l_ptr]() {
    //              p_i->out_path.data = l_ptr->generic_string();
    //              ranges::for_each(p_i->image_to_video_list, [this](impl::image_cache& in_image_cache) {
    //                in_image_cache.out_handle.emplace_or_replace<FSys::path>(p_i->out_path.data);
    //              });
    //            })
    //    );
  }

  if (imgui::Button("选择图片")) {
    auto l_ptr = std::make_shared<std::vector<FSys::path>>();
    //    boost::asio::post(
    //        make_process_adapter<file_dialog>(
    //            strand_gui{g_io_context()},
    //            file_dialog::dialog_args{l_ptr}
    //                .set_title("选择序列"s)
    //                .set_filter(string_list{".png", ".jpg"})
    //        )
    //            .next([this, l_ptr]() {
    //              p_i->image_to_video_list.emplace_back(
    //                  create_image_to_move_handle(l_ptr->front()),
    //                  *l_ptr,
    //                  l_ptr->front().generic_string()
    //              );
    //            })
    //    );
  }
  imgui::SameLine();
  if (imgui::Button("选择文件夹")) {
    auto l_ptr = std::make_shared<std::vector<FSys::path>>();
    //    boost::asio::post(
    //        make_process_adapter<file_dialog>(
    //            strand_gui{g_io_context()},
    //            file_dialog::dialog_args{l_ptr}
    //                .set_title("select dir"s)
    //                .set_use_dir()
    //        )
    //            .next([=]() {
    //              ranges::for_each(*l_ptr, [this](const FSys::path& in_path) {
    //                std::vector<FSys::path> list =
    //                    ranges::make_subrange(FSys::directory_iterator{in_path}, FSys::directory_iterator{}) |
    //                    ranges::views::filter([](const FSys::directory_entry& in_file) {
    //                      return FSys::is_regular_file(in_file);
    //                    }) |
    //                    ranges::views::transform([](const FSys::directory_entry& in_file) -> FSys::path {
    //                      return in_file.path();
    //                    }) |
    //                    ranges::to_vector;
    //                p_i->image_to_video_list.emplace_back(
    //                    create_image_to_move_handle(in_path),
    //                    list,
    //                    in_path.generic_string()
    //                );
    //              });
    //            })
    //    );
  }

  imgui::SameLine();
  if (imgui::Button("清除")) {
    p_i->image_to_video_list.clear();
  }
  imgui::SameLine();
  if (imgui::Button("创建视频")) {
    ranges::for_each(p_i->image_to_video_list, [this](const impl::image_cache& in_cache) {
      g_reg()->ctx().get<image_to_move>()->async_create_move(
          in_cache.out_handle, in_cache.image_attr,
          [this, l_h = in_cache.out_handle]() {  /// \brief 在这里我们将合成的视频添加到下一个工具栏中
            auto l_out_path = l_h.get<FSys::path>();
            p_i->video_list.emplace_back(l_out_path.generic_string(), l_out_path.generic_string());
          }
      );
    });
  }

  dear::ListBox{"image_list"} && [this]() {
    for (const auto& i : p_i->image_to_video_list) {
      dear::Selectable(*i.gui_name);
    }
  };

  if (imgui::Button("选择UE文件")) {
    auto l_ptr = std::make_shared<std::vector<FSys::path>>();
    //boost::asio::post(
        // make_process_adapter<file_dialog>(
        //     strand_gui{g_io_context()},
        //     file_dialog::dialog_args{l_ptr}
        //         .set_title("select mp4 file"s)
        //         .add_filter(".mp4")
        // )
        //     .next([=]() {
        //       p_i->video_list |= ranges::actions::push_back(
        //           *l_ptr |
        //           ranges::views::transform([](const FSys::path& in_path) -> impl::video_cache {
        //             return impl::video_cache{in_path.generic_string()};
        //           })
        //       );
        //     })
    //);
    //---------------------
    g_windows_manage().create_windows_arg(

        windows_init_arg{}
            .create<file_dialog>(file_dialog::dialog_args{}.set_use_dir().add_filter(".uproject").async_read([this](const FSys::path &in) {
              p_i->export_path.path = in / "tmp.xlsx";
              p_i->export_path.stem = "tmp";
              p_i->export_path.data = p_i->export_path.path.generic_string();
            }))
            .set_render_type<dear::Popup>()
            .set_title("选择UE文件"s)

    );
  }
  imgui::SameLine();
  if (imgui::Button("清除视频")) {
    p_i->video_list.clear();
  }
  imgui::SameLine();
  if (imgui::Button("连接视频")) {
    auto l_list =
        p_i->video_list |
        ranges::views::transform([this](impl::video_cache& in_cache) -> FSys::path { return in_cache.data; }) |
        ranges::to_vector;

    p_i->out_video_h.remove<episodes>();
    ranges::find_if(p_i->video_list, [this](impl::video_cache& in_cache) -> bool {
      return episodes::analysis_static(p_i->out_video_h, in_cache.data);
    });

    p_i->out_video_h.emplace_or_replace<FSys::path>(p_i->out_path.data);
    p_i->out_video_h.emplace_or_replace<process_message>();
  }

  dear::ListBox{"video_list"} && [this]() {
    for (const auto& i : p_i->video_list) {
      dear::Selectable(*i.gui_name);
    }
  };
  //----------------------
  // Update item list if we changed the number of items
  static const char* template_items_names[] =
    {
        "Banana", "Apple", "Cherry", "Watermelon", "Grapefruit", "Strawberry", "Mango",
        "Kiwi", "Orange", "Pineapple", "Blueberry", "Plum", "Coconut", "Pear", "Apricot"
    };
  static ImVector<MyItem> items;
  static ImVector<int> selection;
  static bool items_need_sort = false;
  static int items_count = IM_ARRAYSIZE(template_items_names) * 2;
  if (items.Size != items_count)
  {
      items.resize(items_count, MyItem());
      for (int n = 0; n < items_count; n++)
      {
          const int template_n = n % IM_ARRAYSIZE(template_items_names);
          MyItem& item = items[n];
          item.ID = n;
          item.Name = template_items_names[template_n];
          item.Quantity = (template_n == 3) ? 10 : (template_n == 4) ? 20 : 0; // Assign default quantities
      }
  }
  const ImDrawList* parent_draw_list = ImGui::GetWindowDrawList();
  const int parent_draw_list_draw_cmd_count = parent_draw_list->CmdBuffer.Size;
  ImVec2 table_scroll_cur, table_scroll_max; // For debug display
  const ImDrawList* table_draw_list = NULL;  // "
  //-----------------------------
  static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
  static ImGuiTableColumnFlags columns_base_flags = ImGuiTableColumnFlags_None;
  static int freeze_cols = 1;
  static int freeze_rows = 1;
  enum ContentsType { CT_Text, CT_Button, CT_SmallButton, CT_FillButton, CT_Selectable, CT_SelectableSpanRow };
  static int contents_type = CT_SelectableSpanRow;
  static float row_min_height = 0.0f; // Auto    
  static bool show_wrapped_text = false;
  if (ImGui::BeginTable("table_advanced", 6, flags))
  {
      ImGui::TableSetupColumn("ID",           columns_base_flags | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, MyItemColumnID_ID);
      ImGui::TableSetupColumn("Name",         columns_base_flags | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Name);
      ImGui::TableSetupColumn("Action",       columns_base_flags | ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Action);
      ImGui::TableSetupColumn("Quantity",     columns_base_flags | ImGuiTableColumnFlags_PreferSortDescending, 0.0f, MyItemColumnID_Quantity);
      ImGui::TableSetupColumn("Description",  columns_base_flags | ((flags & ImGuiTableFlags_NoHostExtendX) ? 0 : ImGuiTableColumnFlags_WidthStretch), 0.0f, MyItemColumnID_Description);
      ImGui::TableSetupColumn("Hidden",       columns_base_flags |  ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoSort);
      ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);

      // Sort our data if sort specs have been changed!
      ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
      if (sort_specs && sort_specs->SpecsDirty)
          items_need_sort = true;
      if (sort_specs && items_need_sort && items.Size > 1)
      {
          MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
          sort_specs->SpecsDirty = false;
      }
      items_need_sort = false;
      // Take note of whether we are currently sorting based on the Quantity field,
      // we will use this to trigger sorting when we know the data of this column has been modified.
      const bool sorts_specs_using_quantity = (ImGui::TableGetColumnFlags(3) & ImGuiTableColumnFlags_IsSorted) != 0;

      ImGui::TableHeadersRow();
      //--------------------
      // Show data
      ImGui::PushButtonRepeat(true);
       // Demonstrate using clipper for large vertical lists
      ImGuiListClipper clipper;
      clipper.Begin(items.Size);
      while (clipper.Step())
      {
          for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
          {
              MyItem* item = &items[row_n];
              //if (!filter.PassFilter(item->Name))
              //    continue;
              const bool item_is_selected = selection.contains(item->ID);
              ImGui::PushID(item->ID);
              ImGui::TableNextRow(ImGuiTableRowFlags_None, 0);
              // For the demo purpose we can select among different type of items submitted in the first column
              ImGui::TableSetColumnIndex(0);
              char label[32];
              sprintf(label, "%04d", item->ID);
              if (contents_type == CT_Text)
                  ImGui::TextUnformatted(label);
              else if (contents_type == CT_Button)
                  ImGui::Button(label);
              else if (contents_type == CT_SmallButton)
                  ImGui::SmallButton(label);
              else if (contents_type == CT_FillButton)
                  ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f));
              else if (contents_type == CT_Selectable || contents_type == CT_SelectableSpanRow)
              {
                  ImGuiSelectableFlags selectable_flags = (contents_type == CT_SelectableSpanRow) ? ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap : ImGuiSelectableFlags_None;
                  if (ImGui::Selectable(label, item_is_selected, selectable_flags, ImVec2(0, row_min_height)))
                  {
                      if (ImGui::GetIO().KeyCtrl)
                      {
                          if (item_is_selected)
                              selection.find_erase_unsorted(item->ID);
                          else
                              selection.push_back(item->ID);
                      }
                      else
                      {
                          selection.clear();
                          selection.push_back(item->ID);
                      }
                  }
              }

              if (ImGui::TableSetColumnIndex(1))
                  ImGui::TextUnformatted(item->Name);

              if (ImGui::TableSetColumnIndex(2))
              {
                  if (ImGui::SmallButton("Chop")) { item->Quantity += 1; }
                  if (sorts_specs_using_quantity && ImGui::IsItemDeactivated()) { items_need_sort = true; }
                  ImGui::SameLine();
                  if (ImGui::SmallButton("Eat")) { item->Quantity -= 1; }
                  if (sorts_specs_using_quantity && ImGui::IsItemDeactivated()) { items_need_sort = true; }
              }

              if (ImGui::TableSetColumnIndex(3))
                  ImGui::Text("%d", item->Quantity);

              ImGui::TableSetColumnIndex(4);
              if (show_wrapped_text)
                  ImGui::TextWrapped("Lorem ipsum dolor sit amet");
              else
                  ImGui::Text("Lorem ipsum dolor sit amet");

              if (ImGui::TableSetColumnIndex(5))
                  ImGui::Text("1234");

              ImGui::PopID();
          }
      }
      ImGui::PopButtonRepeat();

      // Store some info to display debug details below
      table_scroll_cur = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
      table_scroll_max = ImVec2(ImGui::GetScrollMaxX(), ImGui::GetScrollMaxY());
      table_draw_list = ImGui::GetWindowDrawList();
      ImGui::EndTable();
  }
  return p_i->open;
}
entt::handle zh_model_upload::create_image_to_move_handle(const FSys::path& in_path) {
  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace<process_message>();
  season::analysis_static(l_h, in_path);
  episodes::analysis_static(l_h, in_path);
  shot::analysis_static(l_h, in_path);
  l_h.emplace_or_replace<FSys::path>(p_i->out_path.data);
  return l_h;
}
const std::string& zh_model_upload::title() const { return p_i->title_name_; }
zh_model_upload::~zh_model_upload() = default;

}  // namespace doodle::gui