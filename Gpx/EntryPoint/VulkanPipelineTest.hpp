#pragma once

void vulkan_pipeline_test()
{
    boole checker;

    // create runtime
    gpx::runtime_desc rt_desc;
    rt_desc.type = gpx::runtime_desc::_type::Vulkan;
    rt_desc.debug_mode = boole::False;
    auto rt = gpx::runtime::build(rt_desc);
    checker = rt->init();
    assert(checker);

    // create window
    gpx::window_desc wnd_desc;
    wnd_desc.name = "Hello Pavilion Nine";
    wnd_desc.width = 1920;
    wnd_desc.height = 1080;
    auto wnd = rt->build_window(wnd_desc);
    checker = wnd->start();
    assert(checker);
    tick::sleep(500);

    // build shader
    gpx::shader_desc shader_desc_1;
    shader_desc_1._type = gpx::shader_type::VERTEX;
    shader_desc_1._shader_name = "test vertex shader 1";
    shader_desc_1._window_name = "Hello Pavilion Nine";
    shader_desc_1._shader_path = "./Shader/Vulkan/test1.vert.spv";
    auto vert_shader = rt->build_shader(shader_desc_1);
    checker = vert_shader.has_value();
    assert(checker);

    gpx::shader_desc shader_desc_2;
    shader_desc_2._type = gpx::shader_type::FRAGMENT;
    shader_desc_2._shader_name = "test fragment shader 1";
    shader_desc_2._window_name = "Hello Pavilion Nine";
    shader_desc_2._shader_path = "./Shader/Vulkan/test1.frag.spv";
    auto frag_shader = rt->build_shader(shader_desc_2);
    checker = frag_shader.has_value();
    assert(checker);

    // register pipeline
    gpx::pipeline_desc pipeline_desc;
    pipeline_desc._window_name = "Hello Pavilion Nine";
    pipeline_desc._pipeline_name = "Pavilion Nine Test Pipeline";
    pipeline_desc._shaders.push_back(vert_shader);
    pipeline_desc._shaders.push_back(frag_shader);

    checker = rt->register_pipeline(pipeline_desc);
    assert(checker);
    tick::sleep(500);

    vert_shader->unload();
    frag_shader->unload();

    tick::sleep(500);

    // release resources
    checker = rt->unregister_pipeline("Pavilion Nine Test Pipeline");
    assert(checker);
    checker = wnd->stop();
    assert(checker);
    checker = rt->uninit();
    assert(checker);
}