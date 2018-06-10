
# Week 2
The main goal of the week is to be able to draw mesh and build around that, do some simple but dirty resource creation.

## Wrap the current demo in Dx12Output, Dx12Instance, Dx12Context (don't be super attached to this), etc...

## RHI initializing, implement `Dx12Context::drawImmediateMesh(span<vertex> verts)`
* `Dx12Output`, `Dx12Instance`, `Dx12Device`, `Dx12Context` first scratch, minimal implementation
	- `Dx12Device::getDefaultContext()`

    - Hard Coded Shader
    - NDC space only (No constant buffers yet)
    - Then use constant buffers

## Draw mesh
* Only vertex color, no textures
* `Dx12Context::drawMesh(Mesh& mesh)`
	- Mesh itself as a high level concept should be supported without much change
	- Resource creation support: `Dx12Device::aquireVertexBuffer`, `Dx12Device::aquireIndexBuffer` (through Dx12Device)
		- Resource creation support: `Dx12Device::aquireRenderBuffer`(through Dx12Device)

* Be able to explain the entire pipeline up to drawing a mesh - what is every Dx12's job, where should they live; 

CommandList *list = CommandPoolObject->create_list)(); 
list->begin();
list->start_pipeline(...);
list->bind_mesh(...); 
list->draw(...);
list->end_pipeline();
list->end(); 

device->dispatch_command_list( list ); 


# Week 3
The main goal of this week is to clean up `Dx12xxx` code, so that all the resource(Fence/RootSignature/PipelineState) creation is from `Device`. Besides, be able to draw a textured mesh.

## self care
* clean up `Dx12Context` so that resource creation all goes into `Dx12Device`
* merge back code from `master` branch

## draw textured mesh
* Texture creation
	- `S<Dx12Texture2D> Dx12Device::createTexture2D(const Blob& data, uvec2 size)`
	- `S<Dx12ShaderResourceView> Dx12Device::createShaderResourceView(Dx12Texture2D& tex, uint mipLevel = 0)`
* Texture binding
	- `void Dx12Context::setTexture(Dx12Texture2D& tex, uint slot)`
* Sampler
	- `S<Dx12Sampler> Dx12Device::createSampler(eTextureSampleMode mode, eTextureWrapMode wrapMode)`

* Revisit pushing that up; 
  * Dx12Texture2D
  

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
