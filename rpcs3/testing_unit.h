// Testing Unit File Format:
// VSTU  signature
// DWORD version
// DWORD version of rpcs3 used to record
// DWORD size of game path
// DWORD size of config file
// BUF with the version string
// BUF with game path
// BUF buffer with the config file

// DWORD chunk type: 0 = frame 1 = pad

// if frame:
// QWORD timestamp of frame(0 for first frame)
// DWORD size of frame buffer
// BUF png encoded frame

// if pad event:
// QWORD timestamp of pad event
// BUF pad buffer

#include <string>
#include <vector>
#include <GL/glew.h>
#include "../Utilities/types.h"
#include "Emu/Cell/Modules/cellPad.h"

#define CURRENT_VSTU_VERSION 0x00010000

enum vstu_tag : u32
{
	tag_frame,
	tag_pad
};

class vstu_chunk
{
public:
	virtual void write_to_file(fs::file& tf_file) = 0;

protected:
	u64 timestamp;
};

class vstu_frame : public vstu_chunk
{
public:
	vstu_frame(u8* buffer, u32 _width, u32 _height, u32 _bpp, u64 _timestamp);
	vstu_frame(const std::vector<u8>& png_buf, u64 _timestamp);

	void write_to_file(fs::file& tf_file) override;

protected:
	std::vector<u8> frame_buf;
	u32 width, height, bpp;
};

class vstu_pad : public vstu_chunk
{
public:
	vstu_pad(u8 *buf, u64 _timestamp);
	void write_to_file(fs::file& tf_file) override;

protected:
	std::array<u8, sizeof(CellPadData)> pad_buf;
};

class testing_unit
{
public:
	testing_unit();
	~testing_unit();

	void handle_config();
	void handle_path(const std::string& path);

	bool load_test(const std::string& s_testfile);
	bool save_test(const std::string& s_testfile);

	void frame_event(u8* buf, u32 _width, u32 _height, u32 _pitch);
	void opengl_frame_event(GLuint image, u32 _width, u32 _height, u32 _pitch);

	void pad_event(bool update, u8* pad_buf);

protected:
	u64 get_timestamp() const;

protected:
	std::vector<std::unique_ptr<vstu_chunk>> chunks; // List of chunks (frames and pad events)

	std::string cur_rpcs3_version;
	std::string cur_path;
	std::string cur_config;

	std::array<std::vector<u8>, 2> frame_bufs;
	u32 current_frame = 0;
	std::array<u8, 4> last_colour;
	u64 timestamp_last_frame = 0;

	const s64 variance_trigger = (10 * 255 / 100); // 10% trigger
	const u64 time_trigger     = 1000000;

	enum class vstu_mode
	{
		vstu_inactive,
		vstu_record,
		vstu_replay
	};
	vstu_mode cur_mode = vstu_mode::vstu_inactive;

	const u8 vstu_signature[4] = {'V', 'S', 'T', 'U'};

	struct vstu_file
	{
		u8 signature[4] = {'V', 'S', 'T', 'U'};
		u32 version     = CURRENT_VSTU_VERSION;
		u32 rpcs3_version_size;
		u32 path_size;
		u32 config_file_size;
	};
};

extern testing_unit tu_core;
