// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceImageInputThumbnailRenderer.h

#pragma once
#include "SubstanceImageInputThumbnailRenderer.generated.h"

UCLASS()
class USubstanceImageInputThumbnailRenderer : public UThumbnailRenderer
{
	GENERATED_UCLASS_BODY()

	/** UThumbnailRenderer Object - Renders the Image Input Thumbnail */
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) override;
};
