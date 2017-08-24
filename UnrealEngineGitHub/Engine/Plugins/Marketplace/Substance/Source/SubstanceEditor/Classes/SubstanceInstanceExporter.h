// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceInstanceExporter.h

#pragma once
#include "SubstanceInstanceExporter.generated.h"

UCLASS()
class USubstanceInstanceExporter : public UExporter
{
	GENERATED_UCLASS_BODY()

	/** Exports Text */
	virtual bool ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags = 0) override;
};
