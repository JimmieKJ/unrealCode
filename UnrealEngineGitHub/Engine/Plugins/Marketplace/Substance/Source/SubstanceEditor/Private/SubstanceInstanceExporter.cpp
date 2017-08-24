// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceInstanceExporter.cpp

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceInstanceExporter.h"
#include "SubstanceCoreHelpers.h"

USubstanceInstanceExporter::USubstanceInstanceExporter(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	SupportedClass = USubstanceGraphInstance::StaticClass();
	bText = true;
	FormatExtension.Add(TEXT("sbsprs"));
	FormatDescription.Add(TEXT("Substance preset"));
}

bool USubstanceInstanceExporter::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags)
{
	Substance::Helpers::ExportPresetFromGraph(CastChecked<USubstanceGraphInstance>(Object));
	return true;
}