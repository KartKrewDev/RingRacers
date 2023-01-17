// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_software.hpp"

#include "../i_video.h"
#include "../v_video.h"

#include "../d_netcmd.h"
#ifdef HAVE_DISCORDRPC
#include "../discord.h"
#endif
#include "../doomstat.h"
#include "../m_avrecorder.h"
#include "../st_stuff.h"
#include "../s_sound.h"
#include "../st_stuff.h"
#include "../v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

static void temp_legacy_finishupdate_draws()
{
	SCR_CalculateFPS();
	if (st_overlay)
	{
		if (cv_songcredits.value)
			HU_DrawSongCredits();

		if (cv_ticrate.value)
			SCR_DisplayTicRate();

		if (cv_showping.value && netgame && (consoleplayer != serverplayer || !server_lagless))
		{
			if (server_lagless)
			{
				if (consoleplayer != serverplayer)
					SCR_DisplayLocalPing();
			}
			else
			{
				for (int player = 1; player < MAXPLAYERS; player++)
				{
					if (D_IsPlayerHumanAndGaming(player))
					{
						SCR_DisplayLocalPing();
						break;
					}
				}
			}
		}
		if (cv_mindelay.value && consoleplayer == serverplayer && Playing())
			SCR_DisplayLocalPing();

		M_AVRecorder_DrawFrameRate();
	}

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

#ifdef HAVE_DISCORDRPC
	if (discordRequestList != NULL)
		ST_AskToJoinEnvelope();
#endif
}

SoftwarePass::SoftwarePass() : Pass()
{
}

SoftwarePass::~SoftwarePass() = default;

void SoftwarePass::prepass(Rhi& rhi)
{
	if (rendermode != render_soft)
	{
		return;
	}

	// Render the player views... or not yet? Needs to be moved out of D_Display in d_main.c
	// Assume it's already been done and vid.buffer contains the composited splitscreen view.
	// In the future though, we will want to treat each player viewport separately for postprocessing.

	temp_legacy_finishupdate_draws();

	// Prepare RHI resources
	if (screen_texture_ && (static_cast<int32_t>(width_) != vid.width || static_cast<int32_t>(height_) != vid.height))
	{
		// Mode changed, recreate texture
		rhi.destroy_texture(screen_texture_);
		screen_texture_ = kNullHandle;
	}

	width_ = vid.width;
	height_ = vid.height;

	if (!screen_texture_)
	{
		screen_texture_ = rhi.create_texture({TextureFormat::kLuminance, width_, height_});
	}

	// If the screen width won't fit the unpack alignment, we need to copy the screen.
	if (width_ % kPixelRowUnpackAlignment > 0)
	{
		std::size_t padded_width = (width_ + (kPixelRowUnpackAlignment - 1)) & !kPixelRowUnpackAlignment;
		copy_buffer_.clear();
		copy_buffer_.reserve(padded_width * height_);
		for (std::size_t y = 0; y < height_; y++)
		{
			for (std::size_t x = 0; x < width_; x++)
			{
				copy_buffer_.push_back(vid.buffer[(width_ * y) + x]);
			}

			// Padding to unpack alignment
			for (std::size_t i = 0; i < padded_width - width_; i++)
			{
				copy_buffer_.push_back(0);
			}
		}
	}
}

void SoftwarePass::transfer(Rhi& rhi, Handle<TransferContext> ctx)
{
	// Upload screen
	tcb::span<const std::byte> screen_span;
	if (width_ % kPixelRowUnpackAlignment > 0)
	{
		screen_span = tcb::as_bytes(tcb::span(copy_buffer_));
	}
	else
	{
		screen_span = tcb::as_bytes(tcb::span(vid.buffer, width_ * height_));
	}

	rhi.update_texture(ctx, screen_texture_, {0, 0, width_, height_}, PixelFormat::kR8, screen_span);
}

void SoftwarePass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
}

void SoftwarePass::postpass(Rhi& rhi)
{
}
