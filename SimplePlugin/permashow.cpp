#include "permashow.hpp"
#include <array>

TreeTab* permashow = nullptr;

TreeEntry* permashow_fontsize;


TreeEntry* permashow_pos_x;
TreeEntry* permashow_pos_y;

TreeEntry* background_color;
TreeEntry* border_color;
TreeEntry* separator_color;
TreeEntry* title_color;
TreeEntry* title_background_color;
TreeEntry* text_color;

const std::int32_t padding_x = 14;
const std::int32_t padding_y = 11;

bool permashow_clicked = false;
bool permashow_update = false;

Permashow Permashow::Instance;

Permashow& GetPermashow()
{
	if (!Permashow::Instance.Initialized)
		Permashow::Instance.Init();

	return Permashow::Instance;
}

void draw_outlined_text(const vector& position, unsigned long color, unsigned long outline_color, int font_size, const std::string& text)
{
	// Draw the outline
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			if (x != 0 || y != 0) {
				draw_manager->add_text_on_screen(position + vector(x, y), outline_color, font_size, "%s", text.c_str());
			}
		}
	}

	// Draw the actual text
	draw_manager->add_text_on_screen(position, color, font_size, "%s", text.c_str());
}


std::map<TreeEntry*, std::string> original_display_names;
void Permashow_MenuElementChange(TreeEntry* element)
{
	auto it = std::find_if(Permashow::Instance.PermashowElements.begin(), Permashow::Instance.PermashowElements.end(), [element](const PermashowElement& perma_element)
		{
			return perma_element.AssignedMenuElement == element;
		});

	if (it == Permashow::Instance.PermashowElements.end())
		return;

	// Store the original display name in the map if it is not already present
	if (original_display_names.find(element) == original_display_names.end()) {
		original_display_names[element] = element->display_name();
	}
	std::string output_text = "";
	if (element->element_type() == TreeEntryType::Hotkey) {
		auto virtual_key = it->AssignedMenuElement->get_int();
		output_text.append("[");
		switch (virtual_key) {
		case 0:
			output_text.append("None");
			break;
		case 4:
			output_text.append("MB3");
			break;
		case 5:
			output_text.append("MB4");
			break;
		case 6:
			output_text.append("MB5");
			break;
		case 7:
			output_text.append("None");
			break;
		case 112:
			output_text.append("F1");
			break;
		case 113:
			output_text.append("F2");
			break;
		case 114:
			output_text.append("F3");
			break;
		case 115:
			output_text.append("F4");
			break;
		case 116:
			output_text.append("F5");
			break;
		case 117:
			output_text.append("F6");
			break;
		case 118:
			output_text.append("F7");
			break;
		case 119:
			output_text.append("F8");
			break;
		case 120:
			output_text.append("F9");
			break;
		case 121:
			output_text.append("F10");
			break;
		case 122:
			output_text.append("F11");
			break;
		case 123:
			output_text.append("F12");
			break;
		default:
			char key = static_cast<char>(virtual_key);
			output_text.append(std::string(1, key));
			break;
		}
		output_text.append("] ");
	}


	// Prepend the hotkey to the original display name
	std::string display_name_with_hotkey = output_text + original_display_names[element];
	element->set_display_name(display_name_with_hotkey);

	// Change the "Enabled" and "Disabled" text to "On" and "Off"
	element->get_bool() ? output_text = "ON" : output_text = "OFF";
	it->Value = output_text;
	it->ValueColor = element->get_bool() ? MAKE_COLOR(80, 255, 140, 255) : MAKE_COLOR(255, 127, 80, 255);

	permashow_update = true;
}



void Permashow_FontChange(TreeEntry*)
{
	permashow_update = true;
}

void Permashow_OnDraw()
{
	if (permashow_update)
	{
		Permashow::Instance.Update();
		permashow_update = false;
	}

	//Update position
	//



	point2 p = game_input->get_game_cursor_pos();

	if (keyboard_state->is_pressed(keyboard_game::mouse1))
	{
		if (!permashow_clicked)
		{
			auto rect = p - point2{ permashow_pos_x->get_int(), permashow_pos_y->get_int() };

			if (rect.x >= 0.f && rect.y >= 0.f && rect.x <= Permashow::Instance.box_size.x && rect.y <= Permashow::Instance.box_size.y)
			{
				Permashow::Instance.drag_offset = rect;
				permashow_clicked = true;
			}
		}
	}
	else
	{
		permashow_clicked = false;
	}

	if (permashow_clicked)
	{
		permashow_pos_x->set_int(p.x - Permashow::Instance.drag_offset.x);
		permashow_pos_y->set_int(p.y - Permashow::Instance.drag_offset.y);
	}

	//Draw permashow
	//
	auto font_size = permashow_fontsize->get_int();

	vector position = vector(permashow_pos_x->get_int(), permashow_pos_y->get_int());
	vector box_size = vector(Permashow::Instance.box_size.x, Permashow::Instance.box_size.y);

	draw_manager->add_filled_rect(position, position + box_size, background_color->get_color());
	draw_manager->add_filled_rect(position, position + Permashow::Instance.TitleBoxSize, title_background_color->get_color());
	draw_manager->add_rect(position, position + Permashow::Instance.TitleBoxSize, border_color->get_color());
	draw_manager->add_rect(position, position + box_size, border_color->get_color());

	vector title_position = position + vector((Permashow::Instance.TitleBoxSize.x - Permashow::Instance.TitleSize.x) / 2.f, (Permashow::Instance.TitleBoxSize.y - Permashow::Instance.TitleSize.y) / 2.f);
	unsigned long outline_color = MAKE_COLOR(33, 3, 33, 199); // Define the outline color
	unsigned long outline_color1 = MAKE_COLOR(0, 0, 0, 255); // Define the outline color
	draw_outlined_text(title_position, title_color->get_color(), outline_color, font_size, Permashow::Instance.Title);
	for (auto& element : Permashow::Instance.PermashowElements) {
		draw_outlined_text(
			position + element.NamePos + vector(0, 5),
			text_color->get_color(),
			outline_color1,
			font_size,
			element.AssignedMenuElement ? element.AssignedMenuElement->display_name().c_str() : element.Name.c_str()
		);

		draw_outlined_text(
			position + element.ValuePos + vector(0, 5),
			element.ValueColor,
			outline_color1,
			font_size,
			element.Value.c_str()
		);
	}
}

void Permashow::Update()
{
	auto font_size = permashow_fontsize->get_int();

	TitleSize = draw_manager->calc_text_size(font_size, "%s", Title.c_str());

	std::int32_t max_name_width = TitleSize.x;
	std::int32_t max_value_width = 0;
	std::int32_t max_text_height = TitleSize.y;

	for (auto& element : PermashowElements)
	{
		auto name_size = draw_manager->calc_text_size(font_size, "%s", element.AssignedMenuElement ? element.AssignedMenuElement->display_name().c_str() : element.Name.c_str());
		auto value_size = draw_manager->calc_text_size(font_size, "%s", element.Value.c_str());

		if (name_size.x > max_name_width)
			max_name_width = name_size.x;

		if (value_size.x > max_value_width)
			max_value_width = value_size.x;

		if (name_size.y > max_text_height)
			max_text_height = name_size.y;

		if (value_size.y > max_text_height)
			max_text_height = value_size.y;
	}

	auto element_height = max_text_height + padding_y;
	auto current_element_y = element_height;

	box_size.x = max_name_width + max_value_width + (padding_x * 3.1);

	for (auto& element : PermashowElements)
	{
		auto name_size = draw_manager->calc_text_size(font_size, "%s", element.AssignedMenuElement ? element.AssignedMenuElement->display_name().c_str() : element.Name.c_str());
		auto value_size = draw_manager->calc_text_size(font_size, "%s", element.Value.c_str());

		element.NamePos = { padding_x / 2, current_element_y + (element_height / 2) - (name_size.y / 2) };
		element.SeperatorPos = { (float)((padding_x / 2) + max_name_width + padding_x), current_element_y + 6.f };
		element.ValuePos = { (float)(padding_x / 2 + max_name_width + (padding_x * 2)), current_element_y + (element_height / 2) - (value_size.y / 2) };

		current_element_y += element_height;
	}

	TitleBoxSize = vector(box_size.x, element_height);
	SeperatorHeight = element_height - 12;
	box_size.y = current_element_y + 12;
}

void Permashow::AddElement(std::string name, TreeEntry* element)
{
	element->set_display_name(name);
	PermashowElements.push_back({ element });

	element->add_property_change_callback(Permashow_MenuElementChange);
	Permashow_MenuElementChange(element);
}

void Permashow::Init(std::string title)
{
	Initialized = true;
	permashow = menu->create_tab("trentaio.permashow", "Trent AIO - Permashow");
	{


		permashow_pos_x = permashow->add_slider("permashow.posx", "Position X", int(renderer->screen_width() * 0.72f), 0, renderer->screen_width());
		permashow_pos_y = permashow->add_slider("permashow.posy", "Position Y", int(renderer->screen_height() * 0.74f), 0, renderer->screen_height());

		permashow_fontsize = permashow->add_slider("permashow.font", "Font size", 14, 9, 25);
		permashow_fontsize->add_property_change_callback(Permashow_FontChange);
		auto colors = permashow->add_tab("permashow.colors", "Colors");
		{
			background_color = colors->add_colorpick("permashow.background_color", "Background", { 11 / 255.f, 61 / 255.f, 145 / 255.f, 1.f }); // Navy Blue (#0B3D91)
			text_color = colors->add_colorpick("permashow.text_color", "Color", { 245 / 255.f, 245 / 255.f, 220 / 255.f, 1.f }); // Beige (#F5F5DC)
			border_color = colors->add_colorpick("permashow.border_color", "Border", { 211 / 255.f, 211 / 255.f, 211 / 255.f, 1.f }); // Light Gray (#D3D3D3)
			separator_color = colors->add_colorpick("permashow.separator_color", "Separator", { 255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f }); // White (#FFFFFF)
			title_color = colors->add_colorpick("permashow.title_color", "Title", { 255 / 255.f, 215 / 255.f, 0 / 255.f, 1.f }); // Gold (#FFD700)
			title_background_color = colors->add_colorpick("permashow.title_background_color", "Title Background", { 11 / 255.f, 61 / 255.f, 145 / 255.f, 1.f }); // Navy Blue (#0B3D91)
		}
	}

	Permashow::Instance.Title = title;
	event_handler<events::on_draw>::add_callback(Permashow_OnDraw);
}

void Permashow::Destroy()
{
	if (permashow)
		menu->delete_tab(permashow);
	event_handler<events::on_draw>::remove_handler(Permashow_OnDraw);
}