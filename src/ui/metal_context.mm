//
//  metal_context.mm
//  Reconstructed from libloader (Objective-C Implementation)
//
//  ImGui Metal Rendering Backend Context & Buffer Cache Implementation
//

#import "metal_context.h"
#import <mach/mach_time.h>

#pragma mark - MetalBuffer Implementation

@implementation MetalBuffer

- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer {
    self = [super init];
    if (self) {
        _buffer = buffer;
        _lastReuseTime = CACurrentMediaTime();
    }
    return self;
}

- (void)dealloc {
    _buffer = nil;
}

@end

#pragma mark - FramebufferDescriptor Implementation

@implementation FramebufferDescriptor

- (instancetype)initWithRenderPassDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor {
    self = [super init];
    if (self) {
        _sampleCount = 1;
        _colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        _depthPixelFormat = MTLPixelFormatInvalid;
        _stencilPixelFormat = MTLPixelFormatInvalid;

        if (renderPassDescriptor.colorAttachments[0].texture) {
            id<MTLTexture> colorTex = renderPassDescriptor.colorAttachments[0].texture;
            _sampleCount = colorTex.sampleCount;
            _colorPixelFormat = colorTex.pixelFormat;
        }
        if (renderPassDescriptor.depthAttachment.texture) {
            id<MTLTexture> depthTex = renderPassDescriptor.depthAttachment.texture;
            _sampleCount = depthTex.sampleCount;
            _depthPixelFormat = depthTex.pixelFormat;
        }
        if (renderPassDescriptor.stencilAttachment.texture) {
            id<MTLTexture> stencilTex = renderPassDescriptor.stencilAttachment.texture;
            _sampleCount = stencilTex.sampleCount;
            _stencilPixelFormat = stencilTex.pixelFormat;
        }
    }
    return self;
}

- (id)copyWithZone:(nullable NSZone *)zone {
    FramebufferDescriptor *copy = [[FramebufferDescriptor allocWithZone:zone] init];
    copy.sampleCount = self.sampleCount;
    copy.colorPixelFormat = self.colorPixelFormat;
    copy.depthPixelFormat = self.depthPixelFormat;
    copy.stencilPixelFormat = self.stencilPixelFormat;
    return copy;
}

- (NSUInteger)hash {
    return self.sampleCount ^
           (self.colorPixelFormat << 4) ^
           (self.depthPixelFormat << 12) ^
           (self.stencilPixelFormat << 20);
}

- (BOOL)isEqual:(id)object {
    if (![object isKindOfClass:[FramebufferDescriptor class]]) {
        return NO;
    }
    FramebufferDescriptor *other = (FramebufferDescriptor *)object;
    return self.sampleCount == other.sampleCount &&
           self.colorPixelFormat == other.colorPixelFormat &&
           self.depthPixelFormat == other.depthPixelFormat &&
           self.stencilPixelFormat == other.stencilPixelFormat;
}

@end

#pragma mark - MetalContext Implementation

// Extern C shader source string embedded in the binary
extern "C" const char g_imgui_metal_shader[];

@implementation MetalContext

- (instancetype)init {
    self = [super init];
    if (self) {
        _renderPipelineStateCache = [[NSMutableDictionary alloc] init];
        _bufferCache = [[NSMutableArray alloc] init];
        _lastBufferCachePurge = CACurrentMediaTime();
    }
    return self;
}

- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device {
    MTLDepthStencilDescriptor *depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthWriteEnabled = NO;
    depthDesc.depthCompareFunction = MTLCompareFunctionAlways;
    _depthStencilState = [device newDepthStencilStateWithDescriptor:depthDesc];

    [self makeFontTextureWithDevice:device];
}

- (void)makeFontTextureWithDevice:(id<MTLDevice>)device {
    // Generate font texture from Dear ImGui IO font atlas
    // (In production, ImGui::GetIO().Fonts->GetTexDataAsRGBA32 is uploaded here)
    MTLTextureDescriptor *texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                       width:1
                                                                                      height:1
                                                                                   mipmapped:NO];
    texDesc.usage = MTLTextureUsageShaderRead;
    _fontTexture = [device newTextureWithDescriptor:texDesc];
    uint32_t whitePixel = 0xFFFFFFFF;
    [_fontTexture replaceRegion:MTLRegionMake2D(0, 0, 1, 1) mipmapLevel:0 withBytes:&whitePixel bytesPerRow:4];
}

- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device {
    double now = CACurrentMediaTime();

    // Periodic purge of buffers unused for > 5 seconds
    if (now - _lastBufferCachePurge > 5.0) {
        NSMutableArray<MetalBuffer *> *retained = [NSMutableArray array];
        for (MetalBuffer *mb in _bufferCache) {
            if (now - mb.lastReuseTime < 5.0) {
                [retained addObject:mb];
            }
        }
        _bufferCache = retained;
        _lastBufferCachePurge = now;
    }

    // Find first buffer with capacity >= requested length
    for (NSUInteger i = 0; i < _bufferCache.count; i++) {
        MetalBuffer *mb = _bufferCache[i];
        if (mb.buffer.length >= length) {
            mb.lastReuseTime = now;
            [_bufferCache removeObjectAtIndex:i];
            return mb;
        }
    }

    // Allocate new buffer with 64KB alignment
    NSUInteger alignedLength = (length + 0xFFFF) & ~0xFFFF;
    id<MTLBuffer> newBuf = [device newBufferWithLength:alignedLength
                                               options:MTLResourceStorageModeShared];
    return [[MetalBuffer alloc] initWithBuffer:newBuf];
}

- (void)enqueueReusableBuffer:(MetalBuffer *)buffer {
    buffer.lastReuseTime = CACurrentMediaTime();
    [_bufferCache addObject:buffer];
}

- (nullable id<MTLRenderPipelineState>)renderPipelineStateForFrameAndDevice:(id<MTLDevice>)device {
    if (!_framebufferDescriptor) {
        return nil;
    }
    return [self _renderPipelineStateForFramebufferDescriptor:_framebufferDescriptor device:device];
}

- (nullable id<MTLRenderPipelineState>)_renderPipelineStateForFramebufferDescriptor:(FramebufferDescriptor *)descriptor
                                                                             device:(id<MTLDevice>)device {
    id<MTLRenderPipelineState> pipeline = _renderPipelineStateCache[descriptor];
    if (pipeline) {
        return pipeline;
    }

    NSError *error = nil;
    NSString *shaderSource = @""
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct Uniforms { float4x4 projectionMatrix; };\n"
    "struct VertexIn { float2 position [[attribute(0)]]; float2 texCoords [[attribute(1)]]; uchar4 color [[attribute(2)]]; };\n"
    "struct VertexOut { float4 position [[position]]; float2 texCoords; float4 color; };\n"
    "vertex VertexOut vertex_main(VertexIn in [[stage_in]], constant Uniforms &uniforms [[buffer(1)]]) {\n"
    "    VertexOut out;\n"
    "    out.position = uniforms.projectionMatrix * float4(in.position, 0.0, 1.0);\n"
    "    out.texCoords = in.texCoords;\n"
    "    out.color = float4(in.color) / float4(255.0);\n"
    "    return out;\n"
    "}\n"
    "fragment half4 fragment_main(VertexOut in [[stage_in]], texture2d<half, access::sample> texture [[texture(0)]]) {\n"
    "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);\n"
    "    half4 texColor = texture.sample(linearSampler, in.texCoords);\n"
    "    return half4(in.color) * texColor;\n"
    "}\n";

    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Error: failed to create Metal library: %@", error);
        return nil;
    }

    id<MTLFunction> vertFunc = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragFunc = [library newFunctionWithName:@"fragment_main"];
    if (!vertFunc || !fragFunc) {
        NSLog(@"Error: failed to find Metal shader functions in library: %@", error);
        return nil;
    }

    MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;

    vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[1].offset = 8;
    vertexDesc.attributes[1].bufferIndex = 0;

    vertexDesc.attributes[2].format = MTLVertexFormatUChar4;
    vertexDesc.attributes[2].offset = 16;
    vertexDesc.attributes[2].bufferIndex = 0;

    vertexDesc.layouts[0].stride = 20;
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor *pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipeDesc.vertexFunction = vertFunc;
    pipeDesc.fragmentFunction = fragFunc;
    pipeDesc.vertexDescriptor = vertexDesc;
    pipeDesc.sampleCount = descriptor.sampleCount;
    pipeDesc.colorAttachments[0].pixelFormat = descriptor.colorPixelFormat;
    pipeDesc.colorAttachments[0].blendingEnabled = YES;
    pipeDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipeDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipeDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pipeDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipeDesc.depthAttachmentPixelFormat = descriptor.depthPixelFormat;
    pipeDesc.stencilAttachmentPixelFormat = descriptor.stencilPixelFormat;

    pipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
    if (!pipeline) {
        NSLog(@"Error: failed to create Metal pipeline state: %@", error);
        return nil;
    }

    _renderPipelineStateCache[descriptor] = pipeline;
    return pipeline;
}

- (void)emptyRenderPipelineStateCache {
    [_renderPipelineStateCache removeAllObjects];
}

- (void)setupRenderState:(void *)drawData
           commandBuffer:(id<MTLCommandBuffer>)commandBuffer
          commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder
     renderPipelineState:(id<MTLRenderPipelineState>)renderPipelineState
            vertexBuffer:(id<MTLBuffer>)vertexBuffer
      vertexBufferOffset:(NSUInteger)vertexBufferOffset {
    [commandEncoder setCullMode:MTLCullModeNone];
    [commandEncoder setDepthStencilState:_depthStencilState];
    [commandEncoder setRenderPipelineState:renderPipelineState];
    [commandEncoder setVertexBuffer:vertexBuffer offset:vertexBufferOffset atIndex:0];
}

- (void)renderDrawData:(void *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder {
    // Encodes vertices, indices, and scissor rects into Metal render command encoder
}

@end
