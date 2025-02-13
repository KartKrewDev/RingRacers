// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VORBIS_HPP__
#define __SRB2_MEDIA_VORBIS_HPP__

#include <array>

#include <vorbis/codec.h>

#include "audio_encoder.hpp"
#include "options.hpp"

namespace srb2::media
{

class VorbisEncoder : public AudioEncoder
{
public:
	static const Options options_;

	VorbisEncoder(Config config);
	~VorbisEncoder();

	virtual void encode(sample_buffer_t samples) override final { analyse(samples); }
	virtual void flush() override final { analyse(); }

	virtual const char* name() const override final { return "Vorbis"; }
	virtual int channels() const override final { return vi_.channels; }
	virtual int sample_rate() const override final { return vi_.rate; }

protected:
	using headers_t = std::array<ogg_packet, 3>;

	headers_t generate_headers();

private:
	vorbis_info vi_;
	vorbis_dsp_state vd_;
	vorbis_block vb_;

	void analyse(sample_buffer_t samples = {});
	void write_packet(ogg_packet* op);
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_VORBIS_HPP__
