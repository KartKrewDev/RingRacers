// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  typedef.h
/// \brief Universally accessible type definitions. This
///        file exists so these types can be used anywhere
///        without needing to include specific headers.

#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TYPEDEF3(tag, id, new_type) \
	typedef tag id new_type

#define TYPEDEF2(struct_id, new_type) \
	TYPEDEF3 (struct, struct_id, new_type)

#define TYPEDEF(id) TYPEDEF2 (id, id)

// am_map.h
TYPEDEF (fpoint_t);
TYPEDEF (fline_t);
TYPEDEF (minigen_t);

// command.h
TYPEDEF (vsbuf_t);
TYPEDEF (CV_PossibleValue_t);
TYPEDEF (consvar_t);
TYPEDEF (xcommand_t);

// d_netcmd.h
TYPEDEF (changeteam_packet_t);
TYPEDEF (changeteam_value_t);
TYPEDEF (scheduleTask_t);
TYPEDEF (staffsync_t);

// discord.h
TYPEDEF (discordRequest_t);

// d_player.h
TYPEDEF (respawnvars_t);
TYPEDEF (botvars_t);
TYPEDEF (roundconditions_t);
TYPEDEF (skybox_t);
TYPEDEF (itemroulette_t);
TYPEDEF (powerupvars_t);
TYPEDEF (icecubevars_t);
TYPEDEF (altview_t);
TYPEDEF (player_t);

// d_clisrv.h
TYPEDEF (clientcmd_pak);
TYPEDEF (client2cmd_pak);
TYPEDEF (client3cmd_pak);
TYPEDEF (client4cmd_pak);
TYPEDEF (servertics_pak);
TYPEDEF (serverconfig_pak);
TYPEDEF (filetx_pak);
TYPEDEF (fileacksegment_t);
TYPEDEF (fileack_pak);
TYPEDEF (player_config_t);
TYPEDEF (clientconfig_pak);
TYPEDEF (serverinfo_pak);
TYPEDEF (serverrefuse_pak);
TYPEDEF (askinfo_pak);
TYPEDEF (msaskinfo_pak);
TYPEDEF (plrinfo);
TYPEDEF (filesneededconfig_pak);
TYPEDEF (doomdata_t);
TYPEDEF (serverelem_t);
TYPEDEF (clientkey_pak);
TYPEDEF (serverchallenge_pak);
TYPEDEF (challengeall_pak);
TYPEDEF (responseall_pak);
TYPEDEF (resultsall_pak);
TYPEDEF (say_pak);
TYPEDEF (reqmapqueue_pak);
TYPEDEF (netinfo_pak);
TYPEDEF (voice_pak);

// d_event.h
TYPEDEF (event_t);

// d_netfil.h
TYPEDEF (fileneeded_t);
TYPEDEF (HTTP_login);
TYPEDEF (luafiletransfer_t);

// d_think.h
TYPEDEF (thinker_t);

// d_ticcmd.h
TYPEDEF (ticcmd_t);

// deh_tables.h
TYPEDEF (actionpointer_t);

// dehacked.h
TYPEDEF (MYFILE);

// domdata.h
TYPEDEF (mapvertex_t);
TYPEDEF (mapsidedef_t);
TYPEDEF (maplinedef_t);
TYPEDEF (mapsector_t);
TYPEDEF (mapsubsector_t);
TYPEDEF (mapseg_t);
TYPEDEF (mapnode_t);
TYPEDEF (mapthing_t);

// doomdef.h
TYPEDEF (skincolor_t);

// doomstat.h
TYPEDEF (precipprops_t);
TYPEDEF (skinrecord_t);
TYPEDEF (unloaded_skin_t);
TYPEDEF (skinreference_t);
TYPEDEF (recordtimes_t);
TYPEDEF (recorddata_t);
TYPEDEF (cupwindata_t);
TYPEDEF (scene_t);
TYPEDEF (cutscene_t);
TYPEDEF (textpage_t);
TYPEDEF (textprompt_t);
TYPEDEF (mappoint_t);
TYPEDEF (quake_t);
TYPEDEF (customoption_t);
TYPEDEF (gametype_t);
TYPEDEF (staffbrief_t);
TYPEDEF (mapheader_t);
TYPEDEF (mapheader_lighting_t);
TYPEDEF (unloaded_mapheader_t);
TYPEDEF (tolinfo_t);
TYPEDEF (cupheader_t);
TYPEDEF (unloaded_cupheader_t);
TYPEDEF (exitcondition_t);
TYPEDEF (darkness_t);
TYPEDEF (musicfade_t);
TYPEDEF (teaminfo_t);

// font.h
TYPEDEF (font_t);

// g_demo.h
TYPEDEF (democharlist_t);
TYPEDEF (menudemo_t);
TYPEDEF (demoghost);

// g_game.h
TYPEDEF (roundentry_t);
TYPEDEF (mapsearchfreq_t);

// i_joy.h
TYPEDEF (JoyType_t);

// i_net.h
TYPEDEF (doomcom_t);
TYPEDEF (holepunch_t);
TYPEDEF (bannednode_t);

// i_system.h
TYPEDEF (JoyFF_t);

// i_time.h
TYPEDEF (timestate_t);

// i_tcp.h
TYPEDEF3 (union, mysockaddr_t, mysockaddr_t);

// info.h
TYPEDEF (state_t);
TYPEDEF (mobjinfo_t);

// k_bheap.h
TYPEDEF (bheapitem_t);
TYPEDEF (bheap_t);

// k_boss.h
TYPEDEF (weakspot_t);

// k_bot.h
TYPEDEF (botprediction_t);
TYPEDEF (botcontroller_t);

// k_brightmap.h
TYPEDEF (brightmapStorage_t);

// k_endcam.h
TYPEDEF (endcam_t);

// k_follower.h
TYPEDEF (follower_t);
TYPEDEF (followercategory_t);

// k_hud.h
TYPEDEF (trackingResult_t);

// k_menu.h
TYPEDEF (menucolor_t);
TYPEDEF (menuitem_t);
TYPEDEF (menu_t);
TYPEDEF (menu_anim_t);
TYPEDEF (menucmd_t);
TYPEDEF (setup_player_colors_t);
TYPEDEF (setup_player_t);
TYPEDEF (modedesc_t);

// k_pathfind.h
TYPEDEF (pathfindnode_t);
TYPEDEF (path_t);
TYPEDEF (pathfindsetup_t);

// k_profiles.h
TYPEDEF (profile_t);

// k_serverstats.h
TYPEDEF (serverplayer_t);

// k_bans.h
TYPEDEF (banrecord_t);

// k_terrain.h
TYPEDEF (t_splash_t);
TYPEDEF (t_footstep_t);
TYPEDEF (t_overlay_t);
TYPEDEF (terrain_t);
TYPEDEF (t_floor_t);

// k_waypoint.h
TYPEDEF (waypoint_t);

// k_rank.h
TYPEDEF (gpRank_level_perplayer_t);
TYPEDEF (gpRank_level_t);
TYPEDEF (gpRank_t);

// k_tally.h
TYPEDEF (level_tally_t);

// k_zvote.h
TYPEDEF (midVote_t);
TYPEDEF (midVoteGUI_t);

// k_mapuser.h
TYPEDEF (mapUserProperty_t);
TYPEDEF (mapUserProperties_t);

// lua_hudlib_drawlist.h
typedef struct huddrawlist_s *huddrawlist_h;

// lua_profile.h
TYPEDEF (lua_timer_t);

// m_aatree.h
TYPEDEF (aatree_t);

// m_cond.h
TYPEDEF (condition_t);
TYPEDEF (conditionset_t);
TYPEDEF (emblem_t);
TYPEDEF (unlockable_t);
TYPEDEF (candata_t);
TYPEDEF (gamedata_t);
TYPEDEF (challengegridextradata_t);

// m_dllist.h
TYPEDEF (mdllistitem_t);

// m_fixed.h
TYPEDEF (vector2_t);
TYPEDEF (vector3_t);
TYPEDEF (matrix_t);

// m_perfstats.h
TYPEDEF (ps_hookinfo_t);
TYPEDEF (ps_botinfo_t);

// m_queue.h
TYPEDEF (mqueueitem_t);
TYPEDEF (mqueue_t);

// mserv.h
TYPEDEF (msg_server_t);
TYPEDEF (msg_ban_t);

// p_local.h
TYPEDEF (camera_t);
TYPEDEF (jingle_t);
TYPEDEF (tm_t);
TYPEDEF (TryMoveResult_t);
TYPEDEF (BasicFF_t);

// p_maputl.h
TYPEDEF (divline_t);
TYPEDEF (intercept_t);
TYPEDEF (opening_t);
TYPEDEF (fofopening_t);

// p_mobj.h
TYPEDEF (mobj_t);
TYPEDEF (precipmobj_t);
TYPEDEF (actioncache_t);

// p_polyobj.h
TYPEDEF (polyobj_t);
TYPEDEF (polymaplink_t);
TYPEDEF (polyrotate_t);
TYPEDEF (polymove_t);
TYPEDEF (polywaypoint_t);
TYPEDEF (polyslidedoor_t);
TYPEDEF (polyswingdoor_t);
TYPEDEF (polydisplace_t);
TYPEDEF (polyrotdisplace_t);
TYPEDEF (polyfade_t);
TYPEDEF (polyrotdata_t);
TYPEDEF (polymovedata_t);
TYPEDEF (polywaypointdata_t);
TYPEDEF (polydoordata_t);
TYPEDEF (polydisplacedata_t);
TYPEDEF (polyrotdisplacedata_t);
TYPEDEF (polyflagdata_t);
TYPEDEF (polyfadedata_t);

// p_saveg.h
TYPEDEF (savedata_t);
TYPEDEF (savedata_cup_t);
TYPEDEF (savebuffer_t);

// p_setup.h
TYPEDEF (levelflat_t);

// p_slopes.h
TYPEDEF (dynlineplanethink_t);
TYPEDEF (dynvertexplanethink_t);

// p_spec.h
TYPEDEF (thinkerdata_t);
TYPEDEF (fireflicker_t);
TYPEDEF (lightflash_t);
TYPEDEF (laserthink_t);
TYPEDEF (strobe_t);
TYPEDEF (glow_t);
TYPEDEF (lightlevel_t);
TYPEDEF (ceiling_t);
TYPEDEF (floormove_t);
TYPEDEF (elevator_t);
TYPEDEF (crumble_t);
TYPEDEF (noenemies_t);
TYPEDEF (continuousfall_t);
TYPEDEF (bouncecheese_t);
TYPEDEF (mariothink_t);
TYPEDEF (mariocheck_t);
TYPEDEF (thwomp_t);
TYPEDEF (floatthink_t);
TYPEDEF (eachtime_t);
TYPEDEF (raise_t);
TYPEDEF (executor_t);
TYPEDEF (scroll_t);
TYPEDEF (friction_t);
TYPEDEF (pusher_t);
TYPEDEF (disappear_t);
TYPEDEF (fade_t);
TYPEDEF (fadecolormap_t);
TYPEDEF (planedisplace_t);
TYPEDEF (activator_t);

// r_data.h
TYPEDEF (lumplist_t);

// r_defs.h
TYPEDEF (cliprange_t);
TYPEDEF (vertex_t);
TYPEDEF (degenmobj_t);
TYPEDEF (light_t);
TYPEDEF (node_t);
TYPEDEF (post_t);
TYPEDEF (drawseg_t);
TYPEDEF (rotsprite_t);
TYPEDEF (patch_t);
TYPEDEF (softwarepatch_t);
TYPEDEF (pic_t);
TYPEDEF (spriteframe_t);
TYPEDEF (spritedef_t);
TYPEDEF (extracolormap_t);
TYPEDEF (ffloor_t);
TYPEDEF (lightlist_t);
TYPEDEF (r_lightlist_t);
TYPEDEF (pslope_t);
TYPEDEF (sector_t);
TYPEDEF (line_t);
TYPEDEF (side_t);
TYPEDEF (subsector_t);
TYPEDEF (msecnode_t);
TYPEDEF (mprecipsecnode_t);
TYPEDEF (lightmap_t);
TYPEDEF (seg_t);

// r_fps.h
TYPEDEF (viewvars_t);
TYPEDEF (interpmobjstate_t);
TYPEDEF (levelinterpolator_t);

// r_picformats.h
TYPEDEF (spriteframepivot_t);
TYPEDEF (spriteinfo_t);

// r_plane.h
TYPEDEF (visplane_t);
TYPEDEF (visffloor_t);

// r_portal.h
TYPEDEF (portal_t);

// r_skins.h
TYPEDEF (skin_t);

// r_splats.h
TYPEDEF (floorsplat_t);

// r_state.h
TYPEDEF (sprcache_t);

// r_textures.h
TYPEDEF (texpatch_t);
TYPEDEF (texture_t);

// r_things.h
TYPEDEF (maskcount_t);
TYPEDEF (vissprite_t);
TYPEDEF (drawnode_t);

// s_sound.h
TYPEDEF (listener_t);
TYPEDEF (channel_t);
TYPEDEF (caption_t);
TYPEDEF (musicdef_t);
TYPEDEF (soundtestsequence_t);
TYPEDEF (musicstack_t);

// screen.h
TYPEDEF (viddef_t);
TYPEDEF (vesa_extra_t);
TYPEDEF (vmode_t);

// sounds.h
TYPEDEF (sfxinfo_t);

// taglist.h
TYPEDEF (taglist_t);
TYPEDEF (taggroup_t);

// v_video.h
TYPEDEF (colorlookup_t);
TYPEDEF (cliprect_t);

// w_wad.h
TYPEDEF (filelump_t);
TYPEDEF (wadinfo_t);
TYPEDEF (lumpinfo_t);
TYPEDEF (virtlump_t);
TYPEDEF (virtres_t);
TYPEDEF (wadfile_t);

#undef TYPEDEF
#undef TYPEDEF2

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TYPEDEF_H__
