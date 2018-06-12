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
The main goal of the week is doing some resource related work, visually, to draw a textured mesh. and maybe I can have ConstBuffer to put in MVP.

## Draw mesh
* Only vertex color, no textures
* `Dx12CommandList::drawMesh(Mesh& mesh)`
	- Mesh itself as a high level concept should be supported without much change
	- to achieve that, introduce a couple of concept around `ID3D12Resource` and creation of certain resource views(VertexBufferView, IndexBufferView)
		- UploadBuffer(debating whether should I expose this concept to the engine to have more chaos)
		- `RenderBuffer` revisiting, some technical decision about resource architecture. Reference pattern: unreal, MiniEngine, Falcor. Maybe something in the middle? need some discussion with Forseth to see how he thinks about.
	- in `drawMesh`, 
		- call Dx12CommandList::setVertexBuffer(uint slot, span<BufferView<VertexBuffer>>)
		- call Dx12CommandList::setIndexBuffer(uint slot, span<BufferView<IndexBuffer>>)

## draw textured mesh(following mentioned api probably gonna change)
* Texture creation
	- `S<Dx12Texture2D> Dx12Device::createTexture2D(const Blob& data, uvec2 size)`
	- `S<Dx12ShaderResourceView> Dx12Device::createShaderResourceView(Dx12Texture2D& tex, uint mipLevel = 0)`
* Texture binding
	- `void Dx12CommandList::setTexture(Dx12Texture2D& tex, uint slot)`
* Sampler
	- `S<Dx12Sampler> Dx12Device::createSampler(eTextureSampleMode mode, eTextureWrapMode wrapMode)`
  

# Week 4
The main goal of the week is to have an idea how to update `ShaderProgram` class and start lifting all class one level up.
## RHI layer
* `RHIOutput`, `RHIDevice`, `RHIContext`
	- Dx12XXX should all derive from corresponding interface
* RHIResources
	- create all resource interface, Dx12 specific implementations derive from interfaces.

## Material:  helper classes -> ShaderStage & ShaderProgram (stretch: shader pass)
* implement class `Dx12ShaderStage`, `ShaderStage* RHIDevice::aquireShaderStage(const Blob& code)`
* revisit `ShaderProgram` as interface, implement class `Dx12ShaderProgram`
* `void RHIContext::setMaterial(const Shader& shader, uint pass)`(update current pipeline state object)

# Week 5
Frame buffer
## Frame Buffer
* revisit `FrameBuffer`, implement `Dx12FrameBuffer`.
	- `S<RenderTargetView> RHIDevice::createRenderTargetView(Texture2D& tex)`
	- `void FrameBuffer::setColorTarget(Texture2D* tex, uint slot)`(update current root signature)
	- `void FrameBuffer::setDepthTarget(Texture2D* tex)`(update current root signature)

* revisit `Camera` class, make sure that it can bind multiple render target.

## Material
* Shader reflection support(possibly will be very different from GL)
* Ideally Material should just work
* Revisit pushing renderer up -> may do this now; 
  
# Week 6
Implement Phong shading in HLSL, screen space ambient occulation, update forward render pass

# Week 7
Integrating DXR

# Week 8
using DXR to do ray tracing AO, and using that AO texture.

# Stechgoals?
