// Copyright 2017 The NXT Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Utils.h"

nxt::Device device;
nxt::Queue queue;
nxt::Pipeline pipeline;
nxt::RenderPass renderpass;
nxt::Framebuffer framebuffer;
nxt::Buffer buffer;
nxt::BindGroup bindGroup;

struct {uint32_t a; float b;} s;

void init() {
    device = CreateCppNXTDevice();

    queue = device.CreateQueueBuilder().GetResult();

    nxt::ShaderModule vsModule = CreateShaderModule(device, nxt::ShaderStage::Vertex, R"(
        #version 450
        const vec2 pos[3] = vec2[3](vec2(0.0f, 0.5f), vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f));
        void main() {
            gl_Position = vec4(pos[gl_VertexIndex], 0.5, 1.0);
        })"
    );

    nxt::ShaderModule fsModule = CreateShaderModule(device, nxt::ShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform myBlock {
            int a;
            float b;
        } myUbo;
        out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0, myUbo.a / 255.0, myUbo.b, 1.0);
        })"
    );

    nxt::BindGroupLayout bgl = device.CreateBindGroupLayoutBuilder()
        .SetBindingsType(nxt::ShaderStageBit::Fragment, nxt::BindingType::UniformBuffer, 0, 1)
        .GetResult();

    nxt::PipelineLayout pl = device.CreatePipelineLayoutBuilder()
        .SetBindGroupLayout(0, bgl)
        .GetResult();

    CreateDefaultRenderPass(device, &renderpass, &framebuffer);
    pipeline = device.CreatePipelineBuilder()
        .SetSubpass(renderpass, 0)
        .SetLayout(pl)
        .SetStage(nxt::ShaderStage::Vertex, vsModule, "main")
        .SetStage(nxt::ShaderStage::Fragment, fsModule, "main")
        .GetResult();

    buffer = device.CreateBufferBuilder()
        .SetAllowedUsage(nxt::BufferUsageBit::Mapped | nxt::BufferUsageBit::Uniform)
        .SetInitialUsage(nxt::BufferUsageBit::Mapped)
        .SetSize(sizeof(s))
        .GetResult();

    nxt::BufferView view = buffer.CreateBufferViewBuilder()
        .SetExtent(0, sizeof(s))
        .GetResult();

    bindGroup = device.CreateBindGroupBuilder()
        .SetLayout(bgl)
        .SetUsage(nxt::BindGroupUsage::Frozen)
        .SetBufferViews(0, 1, &view)
        .GetResult();
}

void frame() {
    s.a = (s.a + 1) % 256;
    s.b += 0.02;
    if (s.b >= 1.0f) {s.b = 0.0f;}

    buffer.TransitionUsage(nxt::BufferUsageBit::Mapped);
    buffer.SetSubData(0, sizeof(s) / sizeof(uint32_t), reinterpret_cast<uint32_t*>(&s));

    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderpass, framebuffer)
            .SetPipeline(pipeline)
            .TransitionBufferUsage(buffer, nxt::BufferUsageBit::Uniform)
            .SetBindGroup(0, bindGroup)
            .DrawArrays(3, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);
    DoSwapBuffers();
}

int main(int argc, const char* argv[]) {
    if (!InitUtils(argc, argv)) {
        return 1;
    }
    init();

    while (!ShouldQuit()) {
        frame();
        USleep(16000);
    }

    // TODO release stuff
}
