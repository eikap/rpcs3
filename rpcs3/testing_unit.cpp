#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/RSX/GL/GLHelpers.h"
#include "../3rdparty/libpng/png.h"

#include "testing_unit.h"
#include "rpcs3_version.h"

LOG_CHANNEL(tu_log);

testing_unit tu_core;

vstu_frame::vstu_frame(u8* buffer, u32 _width, u32 _height, u32 _bpp, u64 _timestamp)
{
	width  = _width;
	height = _height;
	bpp    = _bpp;

	frame_buf.resize(width * height * bpp);
	memcpy(frame_buf.data(), buffer, width * height * bpp);

	timestamp = _timestamp;
}

vstu_frame::vstu_frame(const std::vector<u8>& png_buf, u64 _timestamp)
{
	png_structp read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info_ptr   = png_create_info_struct(read_ptr);

	std::pair<const std::vector<u8>&, u32> png_data{png_buf, 0};

	png_set_read_fn(read_ptr, &png_data, [](png_structp png_ptr, png_bytep out, png_size_t to_read) {
		std::pair<const std::vector<u8>&, u32>* p = (std::pair<const std::vector<u8>&, u32>*)png_get_io_ptr(png_ptr);
		if ((p->second + to_read) > p->first.size())
			return;

		memcpy(out, p->first.data() + p->second, to_read);
		p->second += to_read;
	});

	png_uint_32 retval = png_get_IHDR(read_ptr, info_ptr, &width, &height, nullptr, nullptr, nullptr, nullptr, nullptr);
	frame_buf.resize(width * height * 4);

	const png_uint_32 bytes_per_row = png_get_rowbytes(read_ptr, info_ptr);
	for (u32 i = 0; i < height; i++)
	{
		png_read_row(read_ptr, (png_bytep)frame_buf.data() + (i * bytes_per_row), NULL);
	}

	timestamp = _timestamp;
}

void vstu_frame::write_to_file(fs::file& tf_file)
{
	u32 chunk_type = vstu_tag::tag_frame;
	tf_file.write(&chunk_type, sizeof(chunk_type));

	std::vector<u8> encoded_png;

	png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info_ptr    = png_create_info_struct(write_ptr);
	png_set_IHDR(write_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	std::vector<u8*> rows(height);
	for (size_t y = 0; y < height; ++y)
		rows[y] = (u8*)frame_buf.data() + y * width * 4;

	png_set_rows(write_ptr, info_ptr, &rows[0]);
	png_set_write_fn(write_ptr, &encoded_png,
	    [](png_structp png_ptr, png_bytep data, png_size_t length) {
		    std::vector<u8>* p = (std::vector<u8>*)png_get_io_ptr(png_ptr);
		    p->insert(p->end(), data, data + length);
	    },
	    nullptr);
	png_write_png(write_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

	png_destroy_write_struct(&write_ptr, nullptr);

	tf_file.write(&timestamp, sizeof(timestamp));
	u32 encoded_size = encoded_png.size();
	tf_file.write(&encoded_size, sizeof(encoded_size));
	tf_file.write(encoded_png.data(), encoded_size);
}

vstu_pad::vstu_pad(u8* buf, u64 _timestamp)
{
	memcpy(pad_buf.data(), buf, sizeof(CellPadData));
	timestamp = _timestamp;
}

void vstu_pad::write_to_file(fs::file& tf_file)
{
	u32 chunk_type = vstu_tag::tag_pad;
	tf_file.write(&chunk_type, sizeof(chunk_type));
	tf_file.write(pad_buf.data(), sizeof(pad_buf));
}

testing_unit::testing_unit()
{
}

testing_unit::~testing_unit()
{
}

void testing_unit::handle_config()
{
	switch (cur_mode)
	{
	case vstu_mode::vstu_record: cur_config = g_cfg.to_string(); break;
	case vstu_mode::vstu_replay: g_cfg.from_string(cur_config); break;
	case vstu_mode::vstu_inactive: return;
	}
}

void testing_unit::handle_path(const std::string& path)
{
	cur_path = path;
}

bool testing_unit::load_test(const std::string& s_testfile)
{
	chunks.clear();

	fs::file testfile(s_testfile, fs::read);
	if (!testfile)
	{
		tu_log.fatal("Failed to open TU file: %s", s_testfile);
		return false;
	}

	u32 filesize = testfile.size();
	vstu_file loaded_file;

	u32 read_header_size = testfile.read(&loaded_file, sizeof(loaded_file));
	if (read_header_size != sizeof(loaded_file) || memcmp(loaded_file.signature, vstu_signature, 4))
	{
		tu_log.fatal("The signature of the TU file is invalid");
		return false;
	}

	if (loaded_file.version > CURRENT_VSTU_VERSION)
	{
		tu_log.fatal("The TU version of the TU file is unsupported");
		return false;
	}

	cur_rpcs3_version.resize(loaded_file.rpcs3_version_size);
	cur_path.resize(loaded_file.path_size);
	cur_config.resize(loaded_file.config_file_size);

	u32 read_rpcs3_size  = testfile.read(cur_rpcs3_version, loaded_file.rpcs3_version_size);
	u32 read_path_size   = testfile.read(cur_path, loaded_file.path_size);
	u32 read_config_size = testfile.read(cur_config, loaded_file.config_file_size);

	if (read_rpcs3_size != loaded_file.rpcs3_version_size || read_path_size != loaded_file.path_size || read_config_size != loaded_file.config_file_size)
	{
		tu_log.fatal("Failed to read TU file header");
		return false;
	}

	u32 cur_index = read_header_size + read_rpcs3_size + read_config_size;

	while (cur_index < filesize)
	{
		u32 tag;
		if (testfile.read(&tag, sizeof(tag)) != sizeof(tag))
		{
			tu_log.fatal("Failed to read tag");
			return false;
		}

		cur_index += sizeof(tag);

		switch (tag)
		{
		case vstu_tag::tag_frame:
		{
			u64 timestamp;
			u32 size_png;
			if (testfile.read(&timestamp, sizeof(timestamp)) != sizeof(timestamp) || testfile.read(&size_png, sizeof(size_png)) != sizeof(size_png))
			{
				tu_log.fatal("Failed to read frame header");
				return false;
			}

			std::vector<u8> png_buf;
			png_buf.resize(size_png);
			if (testfile.read(png_buf.data(), size_png) != size_png)
			{
				tu_log.fatal("Failed to read png data");
				return false;
			}

			chunks.push_back(std::make_unique<vstu_frame>(png_buf, timestamp));

			break;
		}
		case vstu_tag::tag_pad:
		{
			u64 timestamp;
			if (testfile.read(&timestamp, sizeof(timestamp)) != sizeof(timestamp))
			{
				tu_log.fatal("Failed to read pad header");
				return false;
			}

			std::array<u8, sizeof(CellPadData)> pad_buf;
			if(testfile.read(pad_buf.data(), sizeof(pad_buf))!=sizeof(pad_buf))
			{
				tu_log.fatal("Failed to read pad data");
				return false;
			}

			chunks.push_back(std::make_unique<vstu_pad>(pad_buf.data(), timestamp));

			break;
		}
		}
	}

	return true;
}

bool testing_unit::save_test(const std::string& s_testfile)
{
	fs::file testfile(s_testfile, fs::rewrite);

	if (!testfile)
		return false;

	vstu_file new_file;

	const std::string tf_rpcs3_version = rpcs3::version.to_string();

	new_file.rpcs3_version_size = tf_rpcs3_version.size();
	new_file.path_size          = cur_path.size();
	new_file.config_file_size   = cur_config.size();

	testfile.write(&new_file, sizeof(new_file));
	testfile.write(tf_rpcs3_version.data(), new_file.rpcs3_version_size);
	testfile.write(cur_path.data(), new_file.path_size);
	testfile.write(cur_config.data(), new_file.config_file_size);

	for (const auto& chunk : chunks)
	{
		chunk->write_to_file(testfile);
	}

	return true;
}

u64 testing_unit::get_timestamp() const
{
	return get_system_time() - Emu.GetPauseTime();
}

void testing_unit::frame_event(u8* buf, u32 width, u32 height, u32 pitch)
{
	if (cur_mode == vstu_mode::vstu_record)
	{
		u32 bpp = pitch / width;
		ASSERT(bpp == 4);

		s64 results[4] = {};

		for (u32 index = 0; index < width * height; index++)
		{
			for (u32 index_bpp = 0; index_bpp < bpp; index_bpp++)
			{
				results[index_bpp] += buf[(index * bpp) + index_bpp];
			}
		}

		bool trigger_screencap = false;
		for (u32 index_bpp = 0; index_bpp < bpp; index_bpp++)
		{
			results[index_bpp] /= (width * height);
			if (std::abs(results[index_bpp] - last_colour[index_bpp]) > variance_trigger)
				trigger_screencap = true;

			last_colour[index_bpp] = results[index_bpp];
		}

		// Take a screencap if trigger, first frame or 1s passed
		u64 new_timestamp = get_timestamp();
		if (trigger_screencap || current_frame == 0 || (new_timestamp - timestamp_last_frame) > time_trigger)
		{
			timestamp_last_frame = new_timestamp;

			tu_log.fatal("Screencap triggered: %d", trigger_screencap);

			chunks.push_back(std::make_unique<vstu_frame>(buf, width, height, bpp, get_timestamp()));
		}

		current_frame++;
	}
}

void testing_unit::opengl_frame_event(GLuint image, u32 _width, u32 _height, u32 _pitch)
{
	if (cur_mode == vstu_mode::vstu_inactive)
		return;

	u32 frame_index = current_frame & 1;

	if (frame_bufs[frame_index].size() < (_height * _pitch))
		frame_bufs[frame_index].resize(_height * _pitch);

	if (gl::get_driver_caps().ARB_dsa_supported)
		glGetTextureImage(image, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, _height * _pitch, frame_bufs[frame_index].data());
	else
		glGetTextureImageEXT(GL_TEXTURE_2D, GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, frame_bufs[frame_index].data());

	if (glGetError() != GL_NO_ERROR)
	{
		tu_log.fatal("Error glGetTextureImage");
		return;
	}

	frame_event(frame_bufs[frame_index].data(), _width, _height, _pitch);
}

void testing_unit::pad_event(bool update, u8* pad_buf)
{
	if (cur_mode == vstu_mode::vstu_inactive)
		return;

	if (cur_mode == vstu_mode::vstu_record)
	{
		if (!update)
			return;

		// TODO extra checks to not record in case of sixaxis change
		chunks.push_back(std::make_unique<vstu_pad>(pad_buf, get_timestamp()));
	}
}
