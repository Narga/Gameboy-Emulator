#include "display.h"

void Display::init(Memory* _memory)
{
	memory = _memory;

	int scale = 5;
	
	window.create(sf::VideoMode(width, height), "Gameboy Emulator");
	window.setSize(sf::Vector2u(width * scale, height * scale));
	window.setKeyRepeatEnabled(false);

	bg_array.create(160, 144, sf::Color(255, 0, 255));
	sprites_array.create(160, 144, sf::Color(0, 0, 0, 0)); // transparent

	shades_of_gray[0x0] = sf::Color(255, 255, 255); // 0x0 - White
	shades_of_gray[0x1] = sf::Color(198, 198, 198); // 0x1 - Light Gray
	shades_of_gray[0x2] = sf::Color(127, 127, 127); // 0x2 - Drak Gray
	shades_of_gray[0x3] = sf::Color(0, 0, 0);       // 0x3 - Black
}

void Display::render()
{
	if (!is_lcd_enabled())
		return;

	window.clear(sf::Color::Transparent);

	// clear existig sprite data
	sprites_array.create(160, 144, sf::Color::Transparent);

	bool do_sprites    = memory->LCDC.is_bit_set(BIT_1);
	bool do_background = memory->LCDC.is_bit_set(BIT_0);

	if (do_background)
		render_background();

	if (do_sprites)
		render_sprites();

	sf::Texture bg_texture;
	sf::Texture sprites_texture;

	bg_texture.loadFromImage(bg_array);
	sprites_texture.loadFromImage(sprites_array);

	bg_sprite.setTexture(bg_texture);
	sprites_sprite.setTexture(sprites_texture);
	
	window.draw(bg_sprite);
	window.draw(sprites_sprite);

	window.display();
}

void Display::render_background()
{
	bool bg_code_area = memory->LCDC.is_bit_set(BIT_3);

	Address tile_map_location = (bg_code_area) ? 0x9C00 : 0x9800;
	Byte scroll_x = memory->SCX.get();
	Byte scroll_y = memory->SCY.get();
	Byte palette  = memory->BGP.get();

	// For each pixel in the 160x144 display window:
	// 1. Calculate where the pixel resides in the overall 256x256 background map
	// 2. Get the tile ID where that pixel is located
	// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
	// 4. Plot pixel in 160x144 display view

	// Iterate from top to bottom of display screen (y = 0 -> 144)
	for (int y = 0; y < 144; y++)
	{
		// Iterate from left to right of display screen (x = 0 -> 160)
		for (int x = 0; x < 160; x++)
		{
			// 1. Get pixel X,Y in overall background map, offset by ScrollX & Y
			int map_x = (int) scroll_x + x;
			int map_y = (int) scroll_y + y;

			// Adjust map coordinates if they exceed the 256x256 area to loop around
			map_x = (map_x > 255) ? map_x - 255 : map_x;
			map_y = (map_y > 255) ? map_y - 255 : map_y;

			// 2. Get the tile ID where that pixel is located
			int tile_col = floor(map_x / 8);
			int tile_row = floor(map_y / 8);
			int tile_map_id = (tile_row * 32) + tile_col;
			Address loc = tile_map_location + tile_map_id;
			Byte tile_id = memory->read(loc);

			// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
			// 4. Plot pixel in 160x144 display view
			int tile_x_pixel = map_x % 8;
			int tile_y_pixel = map_y % 8;

			// Invert x pixels because they are stored backwards
			tile_x_pixel = abs(tile_x_pixel - 7);

			render_bg_tile_pixel(palette, x, y, tile_x_pixel, tile_y_pixel, tile_id);
		}
	}
}

void Display::render_bg_tile_pixel(Byte palette, int display_x, int display_y, int tile_x, int tile_y, Byte tile_id)
{
	bool bg_char_selection = memory->LCDC.is_bit_set(BIT_4);

	// Figure out where the current background character data is being stored
	// if selection=0 bg area is 0x8800-0x97FF and tile ID is determined by SIGNED -128 to 127
	// 0x9000 represents the zero ID address in that range
	Address bg_data_location = (bg_char_selection) ? 0x8000 : 0x9000;
	Address offset;

	// 0x8000 - 0x8FFF unsigned 
	if (bg_char_selection)
	{
		offset = (tile_id * 16) + bg_data_location;
	}
	// 0x8800 - 0x97FF signed
	else
	{
		Byte_Signed direction = (Byte_Signed) tile_id;
		Byte_2 temp_offset = (bg_data_location) + (direction * 16);
		offset = (Address) temp_offset;
	}

	Byte
		high = memory->read(offset + (tile_y * 2) + 1),
		low  = memory->read(offset + (tile_y * 2));

	sf::Color color = get_pixel_color(palette, low, high, tile_x, false);
	bg_array.setPixel(display_x, display_y, color);
}

void Display::render_sprites()
{
	Address sprite_data_location = 0xFE00;
	Byte palette_0 = memory->OBP0.get();
	Byte palette_1 = memory->OBP1.get();

	bool use_8x16_sprites = memory->LCDC.is_bit_set(BIT_2);

	// 160 bytes of sprite data / 4 bytes per sprite = 40 potential sprites to render maximum
	for (int sprite_id = 0; sprite_id < 40; sprite_id++)
	{
		Address offset = sprite_data_location + (sprite_id * 4);
		int y_pos = ((int) memory->read(offset)) - 16;
		int x_pos = ((int) memory->read(offset + 1)) - 8;

		Byte tile_id = memory->read(offset + 2);
		Byte flags   = memory->read(offset + 3);

		bool use_palette_1 = is_bit_set(flags, BIT_4);
		Byte sprite_palette = (use_palette_1) ? palette_1 : palette_0;

		// If in 8x16 mode, the tile pattern for top is VAL & 0xFE
		// Lower 8x8 tile is VAL | 0x1

		if (use_8x16_sprites)
		{
			tile_id = tile_id & 0xFE;
			Byte tile_id_bottom = tile_id | 0x01;
			render_sprite_tile(sprite_palette, x_pos, y_pos, tile_id, flags);
			render_sprite_tile(sprite_palette, x_pos, y_pos + 8, tile_id_bottom, flags);
		}
		else
		{
			render_sprite_tile(sprite_palette, x_pos, y_pos, tile_id, flags);
		}
	}
}

void Display::render_sprite_tile(Byte palette, int start_x, int start_y, Byte tile_id, Byte flags)
{
	Address sprite_data_location = 0x8000;

	bool priority = is_bit_set(flags, BIT_7);
	bool mirror_y = is_bit_set(flags, BIT_6);
	bool mirror_x = is_bit_set(flags, BIT_5);

	// If set to zero then sprite always rendered above bg
	// If set to 1, sprite is hidden behind the background and window
	// unless the color of the background or window is white, it's then rendered on top
	bool hide_sprite = is_bit_set(flags, BIT_7);

	for (int y = 0; y < 8; y++)
	{
		int offset = (tile_id * 16) + sprite_data_location;

		Byte
			high = memory->read(offset + (y * 2) + 1),
			low  = memory->read(offset + (y * 2));

		for (int x = 0; x < 8; x++)
		{
			int pixel_x = (mirror_x) ? (start_x + x) : (start_x + 7 - x);
			int pixel_y = start_y + y;

			// prevent pixels from being drawn off screen
			sf::Vector2u bounds = sprites_array.getSize();

			if (pixel_x < 0 || pixel_x >= bounds.x)
				continue;
			if (pixel_y < 0 || pixel_y >= bounds.y)
				continue;

			sf::Color color = get_pixel_color(palette, low, high, x, true);

			// If color in bg/window is anything but white, hide the sprite pixel
			sf::Color bg_color = bg_array.getPixel(pixel_x, pixel_y);

			if (priority && bg_color != sf::Color::White)
				return;

			sprites_array.setPixel(pixel_x, pixel_y, color);
		}
	}
}

// Returns the color of a pixel at X bit based on the 2 relevant line bytes
sf::Color Display::get_pixel_color(Byte palette, Byte top, Byte bottom, int bit, bool is_sprite)
{
	// Figure out what colors to apply to each color code based on the palette data
	Byte color_3_shade = (palette >> 6);        // extract bits 7 & 6
	Byte color_2_shade = (palette >> 4) & 0x03; // extract bits 5 & 4
	Byte color_1_shade = (palette >> 2) & 0x03; // extract bits 3 & 2
	Byte color_0_shade = (palette & 0x03);      // extract bits 1 & 0

	// Get color code from the two defining bytes
	Byte first  = (Byte)is_bit_set(top, bit);
	Byte second = (Byte)is_bit_set(bottom, bit);
	Byte color_code = (second << 1) | first;

	sf::Color result;

	switch (color_code)
	{
	case 0x0: return (is_sprite) ? sf::Color::Transparent : shades_of_gray[color_0_shade];
		case 0x1: return shades_of_gray[color_1_shade];
		case 0x2: return shades_of_gray[color_2_shade];
		case 0x3: return shades_of_gray[color_3_shade];
		default:  return sf::Color(255, 0, 255); // error color
	}

	/*
	case 0b11: return sf::Color(8, 24, 32);
	case 0b10: return sf::Color(48, 104, 80);
	case 0b01: return sf::Color(136, 192, 112);
	case 0b00: return sf::Color(224, 248, 208, ((is_sprite) ? 0 : 255));
	default:   return sf::Color(0, 0, 255); */
}

void Display::draw_scanline() {}

bool Display::is_lcd_enabled()
{
	return memory->LCDC.is_bit_set(BIT_7);
}