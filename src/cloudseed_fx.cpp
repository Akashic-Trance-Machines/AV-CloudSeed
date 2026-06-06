//
// cloudseed_fx.cpp
//
// AV-CloudSeed — IAudioFX wrapper for GhostNote CloudSeed reverb.
// Copyright (C) 2026  The Akashic Trance Machines Team
// This file is part of AV-CloudSeed and is licensed under GPL-3.0.
// See ../LICENSE.
//
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#include "cloudseed_fx.h"
#include "CloudSeedCore/DSP/ReverbController.h"
#include <cstring>
#include <cstdio>

#include "presets.h"

// ── Preset names ──────────────────────────────────────────────────────────────

static const char *const s_PresetNames[] =
{
	"Init",
	"Divine Inspiration", "Laws Of Physics",    "Slow Braaam",
	"The Upside Down",    "Big Sound Stage",     "Diffusion Cyclone",
	"Scream Into Void",   "90s Digital",         "Airy Ambience",
	"Dark Plate",         "Ghostly",             "Tapped Lines",
	"Fast Attack",        "Small Plate",         "Snappy Attack",
};
static constexpr uint16_t NUM_PRESETS = CS_NUM_PRESETS;

// ── Parameter table ───────────────────────────────────────────────────────────
// Param 0 = Preset selector (meta-param, not a CloudSeed parameter).
// Params 1-45 = CloudSeed::Parameter::0-44 in order.

static const TParamDesc s_Params[] =
{
	// pId               pLabel            Type              Display            fMin  fMax    fDef  fStep  ppOpt          nOpt
	{ "preset",         "Preset",         ParamType::Enum,  ParamDisplay::Raw, 0.0f, 15.0f,  0.0f, 1.0f, s_PresetNames, NUM_PRESETS },
	// ── CloudSeed parameters (index = CloudSeed param id + 1) ────────────────
	{ "interpolation",  "Interpolation",  ParamType::Bool,  ParamDisplay::OnOff, 0,1, 0,1, nullptr,0 },	// 0
	{ "low_cut_en",     "Low Cut En",     ParamType::Bool,  ParamDisplay::OnOff, 0,1, 0,1, nullptr,0 },	// 1
	{ "high_cut_en",    "High Cut En",    ParamType::Bool,  ParamDisplay::OnOff, 0,1, 0,1, nullptr,0 },	// 2
	{ "input_mix",      "Input Mix",      ParamType::Float, ParamDisplay::Percent,0,1,1,0.01f,nullptr,0 },	// 3
	{ "low_cut",        "Low Cut",        ParamType::Float, ParamDisplay::Hertz, 0,1,0,0.01f,nullptr,0 },	// 4
	{ "high_cut",       "High Cut",       ParamType::Float, ParamDisplay::Hertz, 0,1,1,0.01f,nullptr,0 },	// 5
	{ "dry_out",        "Dry Out",        ParamType::Float, ParamDisplay::Decibels,0,1,1,0.01f,nullptr,0 }, // 6
	{ "early_out",      "Early Out",      ParamType::Float, ParamDisplay::Decibels,0,1,0,0.01f,nullptr,0 }, // 7
	{ "late_out",       "Late Out",       ParamType::Float, ParamDisplay::Decibels,0,1,1,0.01f,nullptr,0 }, // 8
	{ "tap_en",         "Tap En",         ParamType::Bool,  ParamDisplay::OnOff, 0,1, 1,1, nullptr,0 },	// 9
	{ "tap_count",      "Tap Count",      ParamType::Float, ParamDisplay::Raw, 0,1,0.5f,0.01f,nullptr,0 }, // 10
	{ "tap_decay",      "Tap Decay",      ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0},// 11
	{ "tap_predelay",   "Tap Predelay",   ParamType::Float, ParamDisplay::Milliseconds,0,1,0,0.01f,nullptr,0 },// 12
	{ "tap_length",     "Tap Length",     ParamType::Float, ParamDisplay::Milliseconds,0,1,0.5f,0.01f,nullptr,0},// 13
	{ "early_diff_en",  "Early Diff En",  ParamType::Bool,  ParamDisplay::OnOff, 0,1,1,1,nullptr,0 },	// 14
	{ "early_diff_cnt", "Early Diff Cnt", ParamType::Float, ParamDisplay::Raw, 0,1,0.5f,0.01f,nullptr,0 },	// 15
	{ "early_diff_dly", "Early Diff Dly", ParamType::Float, ParamDisplay::Milliseconds,0,1,0.5f,0.01f,nullptr,0},// 16
	{ "early_diff_mod", "Early Diff Mod", ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0},	// 17
	{ "early_diff_fb",  "Early Diff Fb",  ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0 },	// 18
	{ "early_diff_rate","Early Diff Rate",ParamType::Float, ParamDisplay::Hertz, 0,1,0.5f,0.01f,nullptr,0 },	// 19
	{ "late_mode",      "Late Mode",      ParamType::Bool,  ParamDisplay::OnOff, 0,1,0,1,nullptr,0 },	// 20
	{ "late_line_cnt",  "Late Lines",     ParamType::Float, ParamDisplay::Raw,   0,1,0.5f,0.01f,nullptr,0 },	// 21
	{ "late_diff_en",   "Late Diff En",   ParamType::Bool,  ParamDisplay::OnOff, 0,1,1,1,nullptr,0 },	// 22
	{ "late_diff_cnt",  "Late Diff Cnt",  ParamType::Float, ParamDisplay::Raw,   0,1,0.5f,0.01f,nullptr,0 },	// 23
	{ "late_line_size", "Late Line Size", ParamType::Float, ParamDisplay::Milliseconds,0,1,0.5f,0.01f,nullptr,0},// 24
	{ "late_line_mod",  "Late Line Mod",  ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0},	// 25
	{ "late_diff_dly",  "Late Diff Dly",  ParamType::Float, ParamDisplay::Milliseconds,0,1,0.5f,0.01f,nullptr,0},// 26
	{ "late_diff_mod",  "Late Diff Mod",  ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0},	// 27
	{ "late_decay",     "Late Decay",     ParamType::Float, ParamDisplay::Raw,   0,1,0.5f,0.01f,nullptr,0 },	// 28
	{ "late_line_rate", "Late Line Rate", ParamType::Float, ParamDisplay::Hertz, 0,1,0.5f,0.01f,nullptr,0 },	// 29
	{ "late_diff_fb",   "Late Diff Fb",   ParamType::Float, ParamDisplay::Percent,0,1,0.5f,0.01f,nullptr,0},	// 30
	{ "late_diff_rate", "Late Diff Rate", ParamType::Float, ParamDisplay::Hertz, 0,1,0.5f,0.01f,nullptr,0 },	// 31
	{ "eq_low_en",      "EQ Low En",      ParamType::Bool,  ParamDisplay::OnOff, 0,1,0,1,nullptr,0 },	// 32
	{ "eq_high_en",     "EQ High En",     ParamType::Bool,  ParamDisplay::OnOff, 0,1,0,1,nullptr,0 },	// 33
	{ "eq_lp_en",       "EQ LP En",       ParamType::Bool,  ParamDisplay::OnOff, 0,1,0,1,nullptr,0 },	// 34
	{ "eq_low_freq",    "EQ Low Freq",    ParamType::Float, ParamDisplay::Hertz, 0,1,0.5f,0.01f,nullptr,0 },	// 35
	{ "eq_high_freq",   "EQ High Freq",   ParamType::Float, ParamDisplay::Hertz, 0,1,0.5f,0.01f,nullptr,0 },	// 36
	{ "eq_cutoff",      "EQ Cutoff",      ParamType::Float, ParamDisplay::Hertz, 0,1,1.0f,0.01f,nullptr,0 },	// 37
	{ "eq_low_gain",    "EQ Low Gain",    ParamType::Float, ParamDisplay::Decibels,0,1,0.5f,0.01f,nullptr,0},	// 38
	{ "eq_high_gain",   "EQ High Gain",   ParamType::Float, ParamDisplay::Decibels,0,1,0.5f,0.01f,nullptr,0},	// 39
	{ "eq_cross_seed",  "EQ Cross Seed",  ParamType::Float, ParamDisplay::Percent,0,1,0,0.01f,nullptr,0 },	// 40
	{ "seed_tap",       "Seed Tap",       ParamType::Float, ParamDisplay::Raw,   0,1,0,0.01f,nullptr,0 },	// 41
	{ "seed_diffuse",   "Seed Diffuse",   ParamType::Float, ParamDisplay::Raw,   0,1,0,0.01f,nullptr,0 },	// 42
	{ "seed_delay",     "Seed Delay",     ParamType::Float, ParamDisplay::Raw,   0,1,0,0.01f,nullptr,0 },	// 43
	{ "seed_postdiff",  "Seed PostDiff",  ParamType::Float, ParamDisplay::Raw,   0,1,0,0.01f,nullptr,0 },	// 44
};

static constexpr unsigned NUM_PARAMS = sizeof (s_Params) / sizeof (s_Params[0]); // 46
static const TParamDesc s_NullParam = { "", "", ParamType::Float, ParamDisplay::Raw, 0,0,0,0, nullptr, 0 };

// ── Constructor / Destructor ─────────────────────────────────────────────────

CCloudSeedFX::CCloudSeedFX ()
:	m_pReverb (nullptr),
	m_nPreset (0)
{
}

CCloudSeedFX::~CCloudSeedFX ()
{
	delete m_pReverb;
}

// ── IModule ──────────────────────────────────────────────────────────────────

void CCloudSeedFX::Init (unsigned nSampleRate, unsigned /*nMaxBlock*/)
{
	// Heap-allocate the reverb engine (~65MB of delay lines).
	// This is the only allocation; Process() is allocation-free.
	m_pReverb = new Cloudseed::ReverbController ((int) nSampleRate);
	LoadPreset (0);
}

void CCloudSeedFX::Reset ()
{
	if (m_pReverb)
		LoadPreset (m_nPreset);
}

unsigned CCloudSeedFX::NumParams () const { return NUM_PARAMS; }

const TParamDesc &CCloudSeedFX::ParamDesc (unsigned idx) const
{
	return (idx < NUM_PARAMS) ? s_Params[idx] : s_NullParam;
}

TParamValue CCloudSeedFX::GetParam (unsigned idx) const
{
	if (idx == 0) return { (float) m_nPreset };
	unsigned csIdx = idx - 1;
	if (!m_pReverb || csIdx >= 45) return { 0.0f };
	return { (float) m_pReverb->GetAllParameters()[csIdx] };
}

void CCloudSeedFX::SetParam (unsigned idx, TParamValue v)
{
	if (idx == 0)
	{
		int preset = v.AsInt ();
		if (preset < 0)          preset = 0;
		if (preset >= NUM_PRESETS) preset = NUM_PRESETS - 1;
		LoadPreset (preset);
		return;
	}
	unsigned csIdx = idx - 1;
	if (!m_pReverb || csIdx >= 45) return;
	m_pReverb->SetParameter (csIdx, v.f);
}

int CCloudSeedFX::FindParam (const char *pId) const
{
	for (unsigned i = 0; i < NUM_PARAMS; i++)
		if (strcmp (s_Params[i].pId, pId) == 0) return (int) i;
	return -1;
}

size_t CCloudSeedFX::Serialize   (uint8_t *, size_t) const   { return 0; }
size_t CCloudSeedFX::Deserialize (const uint8_t *, size_t)   { return 0; }

// ── IAudioFX ─────────────────────────────────────────────────────────────────

void CCloudSeedFX::Process (float *pIoL, float *pIoR, unsigned nFrames)
{
	if (!m_pReverb) return;
	m_pReverb->Process (pIoL, pIoR, pIoL, pIoR, (int) nFrames);
}

// ── Private ──────────────────────────────────────────────────────────────────

void CCloudSeedFX::LoadPreset (int nPreset)
{
	m_nPreset = nPreset;
	if (!m_pReverb) return;

	const float *data = CSPresets[nPreset];
	for (int i = 0; i < 45; i++)
		m_pReverb->SetParameter (i, data ? data[i] : 0.0f);
}
