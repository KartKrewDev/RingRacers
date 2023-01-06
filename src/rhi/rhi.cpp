#include "rhi.hpp"

#include <exception>
#include <stdexcept>

using namespace srb2;
using namespace srb2::rhi;

Rhi::~Rhi() = default;

const ProgramRequirements srb2::rhi::kProgramRequirementsUnshaded = {
	ProgramVertexInputRequirements {{
		ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}
	}},
	ProgramUniformRequirements {{
		{{UniformName::kProjection}},
		{{UniformName::kModelView, UniformName::kTexCoord0Transform}}
	}},
	ProgramSamplerRequirements {{
		ProgramSamplerInput {SamplerName::kSampler0, true}
	}}
};

const ProgramRequirements srb2::rhi::kProgramRequirementsUnshadedPaletted = {
	ProgramVertexInputRequirements {{
		ProgramVertexInput {VertexAttributeName::kPosition, VertexAttributeFormat::kFloat3, true},
		ProgramVertexInput {VertexAttributeName::kTexCoord0, VertexAttributeFormat::kFloat2, false},
		ProgramVertexInput {VertexAttributeName::kColor, VertexAttributeFormat::kFloat4, false}
	}},
	ProgramUniformRequirements {{
		{{UniformName::kProjection}},
		{{UniformName::kModelView, UniformName::kTexCoord0Transform}}
	}},
	ProgramSamplerRequirements {{
		ProgramSamplerInput {SamplerName::kSampler0, true},
		ProgramSamplerInput {SamplerName::kSampler1, true}
	}}
};

const ProgramRequirements& rhi::program_requirements_for_program(PipelineProgram program) noexcept {
	switch (program) {
	case PipelineProgram::kUnshaded:
		return kProgramRequirementsUnshaded;
	case PipelineProgram::kUnshadedPaletted:
		return kProgramRequirementsUnshadedPaletted;
	default:
		std::terminate();
	}
}
