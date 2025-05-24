// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
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

const ProgramRequirements srb2::rhi::kProgramRequirementsPostimg = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, true}}},
	ProgramUniformRequirements {
		{{
		  {UniformName::kTime, true},
		  {UniformName::kProjection, true},
		  {UniformName::kModelView, true},
		  {UniformName::kTexCoord0Transform, true},
		  {UniformName::kTexCoord0Min, true},
		  {UniformName::kTexCoord0Max, true},
		  {UniformName::kPostimgWater, true},
		  {UniformName::kPostimgHeat, true}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}, {SamplerName::kSampler1, false}}}
};

const ProgramRequirements srb2::rhi::kProgramRequirementsSharpBilinear = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		 ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true}}},
		 {{{UniformName::kModelView, true}, {UniformName::kTexCoord0Transform, true}, {UniformName::kSampler0Size, true}}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}}}};

const ProgramRequirements srb2::rhi::kProgramRequirementsCrt = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		 ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true}}},
		 {{{UniformName::kModelView, true}, {UniformName::kTexCoord0Transform, true}, {UniformName::kSampler0Size, true}}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}, {SamplerName::kSampler1, true}}}};

const ProgramRequirements srb2::rhi::kProgramRequirementsCrtSharp = {
	ProgramVertexInputRequirements {
		{ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		 ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		 ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}}},
	ProgramUniformRequirements {
		{{{{UniformName::kProjection, true}}},
		 {{{UniformName::kModelView, true}, {UniformName::kTexCoord0Transform, true}, {UniformName::kSampler0Size, true}}}}},
	ProgramSamplerRequirements {{{SamplerName::kSampler0, true}, {SamplerName::kSampler1, true}}}};

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
	case PipelineProgram::kPostimg:
		return kProgramRequirementsPostimg;
	case PipelineProgram::kSharpBilinear:
		return kProgramRequirementsSharpBilinear;
	case PipelineProgram::kCrt:
		return kProgramRequirementsCrt;
	case PipelineProgram::kCrtSharp:
		return kProgramRequirementsCrtSharp;
	default:
		std::terminate();
	}
}

bool rhi::recreate_buffer_to_size(Rhi& rhi, Handle<Buffer>& buffer, const BufferDesc& desc)
{
	bool recreate = false;
	if (buffer == kNullHandle)
	{
		recreate = true;
	}
	else
	{
		std::size_t existing_size = rhi.get_buffer_size(buffer);
		if (existing_size < desc.size)
		{
			rhi.destroy_buffer(buffer);
			recreate = true;
		}
	}

	if (recreate)
	{
		buffer = rhi.create_buffer(desc);
	}

	return recreate;
}
