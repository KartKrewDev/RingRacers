// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "rhi.hpp"

#include <exception>
#include <stdexcept>

using namespace srb2;
using namespace srb2::rhi;

Rhi::~Rhi() = default;

const ProgramRequirements srb2::rhi::kProgramRequirementsUnshaded = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		 ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true}}},
		 {{{UniformName::kModelView, true}, {UniformName::kTexCoord0Transform, true}}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}}}};

const ProgramRequirements srb2::rhi::kProgramRequirementsUnshadedPaletted = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		 ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true}}},
		 {{{UniformName::kModelView, true},
		   {UniformName::kTexCoord0Transform, true},
		   {UniformName::kSampler0IsIndexedAlpha, false}}}}},
	ProgramSamplerRequirements {
		{{SamplerName::kSampler0, true}, {SamplerName::kSampler1, true}, {SamplerName::kSampler2, false}}}};

const ProgramRequirements srb2::rhi::kProgramRequirementsPostprocessWipe = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, true}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true},
		   {UniformName::kWipeColorizeMode, true},
		   {UniformName::kWipeEncoreSwizzle, true}}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}, {SamplerName::kSampler1, true}, {SamplerName::kSampler2, true}}}};

const ProgramRequirements& rhi::program_requirements_for_program(PipelineProgram program) noexcept
{
	switch (program)
	{
	case PipelineProgram::kUnshaded:
		return kProgramRequirementsUnshaded;
	case PipelineProgram::kUnshadedPaletted:
		return kProgramRequirementsUnshadedPaletted;
	case PipelineProgram::kPostprocessWipe:
		return kProgramRequirementsPostprocessWipe;
	default:
		std::terminate();
	}
}
