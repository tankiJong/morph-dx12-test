# Week 2
The main goal of the week is to be able to draw mesh and build around that, do some simple but dirty resource creation.

## Wrap the current demo in Dx12Output, Dx12Instance, Dx12Context (don't be super attached to this), etc...

## RHI initializing, implement `Dx12Context::drawImmediateMesh(span<vertex> verts)`
* `Dx12Output`, `Dx12Instance`, `Dx12Device`, `Dx12Context` first scratch, minimal implementation
	- `Dx12Device::getDefaultContext()`

    - Hard Coded Shader
    - NDC space only (No constant buffers yet)
    - Then use constant buffers

## self care
* clean up `Dx12Context` so that resource creation all goes into `Dx12Device`
* merge back code from `master` branch
* perpare tools for geometry data creation(for draw mesh)
	- `RootSignature` class
	- `PipelineState` class

----------------
### Summary: 
Some code snippets:
```cpp
  /* Dx12CommandList::Dx12CommandList(S<Dx12Device>) root Signature creation through `RootSignature` class */
  mRootSignature.reset(new RootSignature(mDevice));
  // mRootSignature[0].asSRV(registerId, shaderVisibility);
  // mRootSignature.setStaticSampler(registerId, samplerDesc, visibility);
  mRootSignature->finalize(L"default", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
```
```cpp
  /* pipeline state usage */
  mPipelineState->setRootSignature(mRootSignature); // pipeline state will need a root signature
  mPipelineState->setInputLayout(*VertexLayout::For<vertex_pcu_t>());
  // mPipelineState->setShader(eShaderStageType, const ShaderPass&)
  // mPipelineState->setRenderState(const render_state&)
  mPipelineState->finalize(); // finalize will trigger the pso creation

```
### Yaas
I feel pretty happy that I have pretty clean way to initialize the RHI layer, and the CommandList(Context) initialization is pretty clean now, which I will possibly go with. At the same time, setInputLayout is pretty nicely integrated with my existing design.

The next step, I will need to toch the `RenderBuffer` part, to figure out a way to create and create resource view(IndexBufferView, VertexBufferView), so that the drawVertexArray can be shorter.

Meanwhile, I also need to care about other resource creation(maybe not managment yet), like texture as well as how to use them(RTV, SRV creation from Resource), basically I will start playing around ID3D12Resource.

### Nope
I do not enough understanding about Fence and everything around it. I will need to read more material(also will really appreciate a discussion around it when we play more with it(possibly it will make sense to you earlier tho))about how exactly what's going on around that object and my fence value during the runtime. This part seems like a time bomb for me now, since inappropriate usage will stall the whole program(which happen to me several times).

I need to learn more about the life cycle about all ID3Dxxx* in terms of when can I delete it? do I need to delete it? For now, every time I close the program trigger an fatal error... which I just ignore for now. I will expect I dig into this part more when I start doing resource management.


----------------

# Week 3
Turn out this week was mainly fighting with the infrastructure, how to do things correctly.
I learned a lot ideas from Falcor and Unreal. I am now mainly following the way Falcor handles the RHI. Main reason is that Falcor provides an clean and pretty "light" way(compared with Unreal) to warpping around the api calls.

-----------------
## Reintroduce RHIDevice, RHIContext
* before, these thing live in the Game project, as an simple layer help me to understand the concepts. This week, I started migrating these things into engine.
* I did several decisions. 
	- I do not have `Output` in my engine. That layer will add additional complexity which I do not really care for now. Device will be considered as Device+Output
	- One device will only one context
	- Unlike Falcor, I will only have one type of Context, which is the `RenderContext` in Falcor, i do not need this layer of inheritance and this pretty easy to change later.

## Yaas
* A lot of concept wrapper are residents of my engine now
* For resource, I have `RHIContext::updateBuffer`, `RHIBuffer::create` two parts ready. Build around that, `RHIContext::resourceBarrier` ready

## Nope
* I do not even have a triangle again now, which is really really sad
* As a reason of the previous point, I feel it's very hard for me to do things elegant and also get it working very fast. To achieve A, I will need to build B, which will need C, which will need D and E... But fortunately, I am almost done with pains.

-----------------
# Week 4
* DrawMesh
	- `PipelineState::setVertexBuffer(RHIBuffer*)`, `PipelineState::setIndexBuffer(RHIBuffer*)`

* setTexture
	- some fun stuffs around resource heap(first try)
	- draw a textured mesh

-----------------
## Yaas
* got texture up and I pretty like the structure for creating and using resource I have now

## Nope
* Texture creation is hard coded now, only support 2d, 4 components
* frame buffer is kinda spaghetti now just a whole mess in `beginFrame`
-----------------

# Week 5
* Constant Buffer & CBV; SRV
	- build around that, have some Resource view infrustructure in
	- DescriptorSet can say setSrv(), setCbv()...
* Defer Release for GPU resource

# Week 6
* Hello Cube 
	- re-introduce FBO, Mesh, Camera, scratch for immediate renderer
	- drawMesh
* Shader support
	- architecture research

# Week 7
...

# Week 8
...

