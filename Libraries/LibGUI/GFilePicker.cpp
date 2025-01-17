#include <AK/FileSystemPath.h>
#include <LibGUI/GAction.h>
#include <LibGUI/GBoxLayout.h>
#include <LibGUI/GButton.h>
#include <LibGUI/GDirectoryModel.h>
#include <LibGUI/GFilePicker.h>
#include <LibGUI/GInputBox.h>
#include <LibGUI/GLabel.h>
#include <LibGUI/GMessageBox.h>
#include <LibGUI/GSortingProxyModel.h>
#include <LibGUI/GTextBox.h>
#include <LibGUI/GToolBar.h>
#include <SharedGraphics/PNGLoader.h>

GFilePicker::GFilePicker(const StringView& path, CObject* parent)
    : GDialog(parent)
    , m_model(GDirectoryModel::create())
{
    set_title("GFilePicker");
    set_rect(200, 200, 700, 400);
    auto* horizontal_container = new GWidget;
    set_main_widget(horizontal_container);
    horizontal_container->set_layout(make<GBoxLayout>(Orientation::Horizontal));
    horizontal_container->layout()->set_margins({ 4, 4, 4, 4 });
    horizontal_container->set_fill_with_background_color(true);
    horizontal_container->set_background_color(Color::WarmGray);

    auto* vertical_container = new GWidget(horizontal_container);
    vertical_container->set_layout(make<GBoxLayout>(Orientation::Vertical));
    vertical_container->layout()->set_spacing(4);

    auto* upper_container = new GWidget(vertical_container);
    upper_container->set_layout(make<GBoxLayout>(Orientation::Horizontal));
    upper_container->layout()->set_spacing(4);
    upper_container->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    upper_container->set_preferred_size({ 0, 26 });

    auto* toolbar = new GToolBar(upper_container);
    toolbar->set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
    toolbar->set_preferred_size({ 60, 0 });
    toolbar->set_has_frame(false);

    auto* location_textbox = new GTextBox(upper_container);
    location_textbox->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    location_textbox->set_preferred_size({ 0, 20 });

    m_view = new GTableView(vertical_container);
    m_view->set_model(GSortingProxyModel::create(*m_model));
    m_view->set_column_hidden(GDirectoryModel::Column::Owner, true);
    m_view->set_column_hidden(GDirectoryModel::Column::Group, true);
    m_view->set_column_hidden(GDirectoryModel::Column::Permissions, true);
    m_view->set_column_hidden(GDirectoryModel::Column::Inode, true);
    m_model->open(path);

    location_textbox->on_return_pressed = [&] {
        m_model->open(location_textbox->text());
        clear_preview();
    };

    auto open_parent_directory_action = GAction::create("Open parent directory", { Mod_Alt, Key_Up }, GraphicsBitmap::load_from_file("/res/icons/16x16/open-parent-directory.png"), [this](const GAction&) {
        m_model->open(String::format("%s/..", m_model->path().characters()));
        clear_preview();
    });
    toolbar->add_action(*open_parent_directory_action);

    auto mkdir_action = GAction::create("New directory...", GraphicsBitmap::load_from_file("/res/icons/16x16/mkdir.png"), [this](const GAction&) {
        GInputBox input_box("Enter name:", "New directory", this);
        if (input_box.exec() == GInputBox::ExecOK && !input_box.text_value().is_empty()) {
            auto new_dir_path = FileSystemPath(String::format("%s/%s",
                                                   m_model->path().characters(),
                                                   input_box.text_value().characters()))
                                    .string();
            int rc = mkdir(new_dir_path.characters(), 0777);
            if (rc < 0) {
                GMessageBox::show(String::format("mkdir(\"%s\") failed: %s", new_dir_path.characters(), strerror(errno)), "Error", GMessageBox::Type::Error, this);
            } else {
                m_model->update();
            }
        }
    });
    toolbar->add_action(*mkdir_action);

    auto* lower_container = new GWidget(vertical_container);
    lower_container->set_layout(make<GBoxLayout>(Orientation::Vertical));
    lower_container->layout()->set_spacing(4);
    lower_container->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    lower_container->set_preferred_size({ 0, 60 });

    auto* filename_container = new GWidget(lower_container);
    filename_container->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    filename_container->set_preferred_size({ 0, 20 });
    filename_container->set_layout(make<GBoxLayout>(Orientation::Horizontal));
    auto* filename_label = new GLabel("File name:", filename_container);
    filename_label->set_text_alignment(TextAlignment::CenterLeft);
    filename_label->set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
    filename_label->set_preferred_size({ 60, 0 });
    auto* filename_textbox = new GTextBox(filename_container);

    m_view->on_activation = [this, filename_textbox](auto& index) {
        auto& filter_model = (GSortingProxyModel&)*m_view->model();
        auto local_index = filter_model.map_to_target(index);
        const GDirectoryModel::Entry& entry = m_model->entry(local_index.row());

        FileSystemPath path(String::format("%s/%s", m_model->path().characters(), entry.name.characters()));

        clear_preview();

        if (entry.is_directory()) {
            m_model->open(path.string());
            // NOTE: 'entry' is invalid from here on
        } else {
            filename_textbox->set_text(entry.name);
            set_preview(path);
        }
    };

    auto* button_container = new GWidget(lower_container);
    button_container->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    button_container->set_preferred_size({ 0, 20 });
    button_container->set_layout(make<GBoxLayout>(Orientation::Horizontal));
    button_container->layout()->set_spacing(4);
    button_container->layout()->add_spacer();

    auto* cancel_button = new GButton(button_container);
    cancel_button->set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
    cancel_button->set_preferred_size({ 80, 0 });
    cancel_button->set_text("Cancel");
    cancel_button->on_click = [this](auto&) {
        done(ExecCancel);
    };

    auto* ok_button = new GButton(button_container);
    ok_button->set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
    ok_button->set_preferred_size({ 80, 0 });
    ok_button->set_text("OK");
    ok_button->on_click = [this, filename_textbox](auto&) {
        FileSystemPath path(String::format("%s/%s", m_model->path().characters(), filename_textbox->text().characters()));
        m_selected_file = path;
        done(ExecOK);
    };

    auto* preview_container = new GFrame(horizontal_container);
    preview_container->set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
    preview_container->set_preferred_size({ 180, 0 });
    preview_container->set_frame_shape(FrameShape::Container);
    preview_container->set_frame_shadow(FrameShadow::Sunken);
    preview_container->set_frame_thickness(2);
    preview_container->set_layout(make<GBoxLayout>(Orientation::Vertical));
    preview_container->layout()->set_margins({ 8, 8, 8, 8 });

    m_preview_image_label = new GLabel(preview_container);
    m_preview_image_label->set_should_stretch_icon(true);
    m_preview_image_label->set_size_policy(SizePolicy::Fixed, SizePolicy::Fixed);
    m_preview_image_label->set_preferred_size({ 160, 160 });

    m_preview_name_label = new GLabel(preview_container);
    m_preview_name_label->set_font(Font::default_bold_font());
    m_preview_name_label->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    m_preview_name_label->set_preferred_size({ 0, m_preview_name_label->font().glyph_height() });

    m_preview_geometry_label = new GLabel(preview_container);
    m_preview_geometry_label->set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
    m_preview_geometry_label->set_preferred_size({ 0, m_preview_name_label->font().glyph_height() });
}

GFilePicker::~GFilePicker()
{
}

void GFilePicker::set_preview(const FileSystemPath& path)
{
    if (path.has_extension(".png")) {
        auto bitmap = load_png(path.string());
        if (!bitmap) {
            clear_preview();
            return;
        }
        bool should_stretch = bitmap->width() > m_preview_image_label->width() || bitmap->height() > m_preview_image_label->height();
        m_preview_name_label->set_text(path.basename());
        m_preview_geometry_label->set_text(bitmap->size().to_string());
        m_preview_image_label->set_should_stretch_icon(should_stretch);
        m_preview_image_label->set_icon(move(bitmap));
    }
}

void GFilePicker::clear_preview()
{
    m_preview_image_label->set_icon(nullptr);
    m_preview_name_label->set_text(String::empty());
    m_preview_geometry_label->set_text(String::empty());
}
