﻿#include "stdafx.h"
#include <gmm.h>

gm::IAudioReader* GMMFactory::getAudioReader()
{
	// 必须要先初始化播放器
	getAudioPlayer();

	static GMMAudioReader s;
	return &s;
}

gm::IAudioPlayer* GMMFactory::getAudioPlayer()
{
	static GMMAudioPlayer s;
	return &s;
}