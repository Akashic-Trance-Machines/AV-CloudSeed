//
// cloudseed_fx.h
//
// AV-CloudSeed — IAudioFX wrapper for GhostNote CloudSeed reverb.
// Exposes all 45 CloudSeed parameters + a Preset selector.
//
// Copyright (C) 2026  The Akashic Trance Machines Team
// This file is part of AV-CloudSeed and is licensed under GPL-3.0.
// See ../LICENSE.
//
#pragma once

// BUFFER_SIZE must be defined before including CloudSeedCore headers.
// Sets the internal sub-block size; we match our 256-frame host block.
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#include "engine/iaudiofx.h"

// Forward-declare to avoid pulling 65MB of static arrays into every TU.
namespace Cloudseed { class ReverbController; }

class CCloudSeedFX : public IAudioFX
{
public:
	CCloudSeedFX ();
	~CCloudSeedFX () override;

	// IModule
	const char	*Id ()   const override { return "cloudseed"; }
	const char	*Name () const override { return "CloudSeed"; }

	void	Init   (unsigned nSampleRate, unsigned nMaxBlock) override;
	void	Reset  () override;

	unsigned		 NumParams ()              const override;
	const TParamDesc	&ParamDesc (unsigned idx)  const override;
	TParamValue		 GetParam  (unsigned idx)  const override;
	void			 SetParam  (unsigned idx, TParamValue v) override;
	int			 FindParam (const char *pId) const override;

	size_t	Serialize   (uint8_t *pBuf, size_t nCap) const override;
	size_t	Deserialize (const uint8_t *pBuf, size_t nLen) override;

	// IAudioFX
	void     Process    (float *pIoL, float *pIoR, unsigned nFrames) override;
	unsigned TailFrames () const override { return 48000 * 10; } // 10s tail

private:
	void LoadPreset (int nPreset);

private:
	Cloudseed::ReverbController *m_pReverb;	// heap-allocated in Init()
	int	m_nPreset;			// 0-15
};
