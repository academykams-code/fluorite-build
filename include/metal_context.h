//
//  metal_context.h
//  Reconstructed from libloader (Objective-C Runtime Metadata)
//
//  ImGui Metal Rendering Backend Context & Buffer Cache
//

#pragma once

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * MetalBuffer wraps an MTLBuffer with timestamp tracking for LRU cache eviction and reuse.
 */
@interface MetalBuffer : NSObject

@property (nonatomic, strong) id<MTLBuffer> buffer;
@property (nonatomic, assign) double lastReuseTime;

- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer;

@end

/**
 * FramebufferDescriptor captures render target pixel formats and MSAA sample count
 * to look up or compile matching MTLRenderPipelineState instances.
 */
@interface FramebufferDescriptor : NSObject <NSCopying>

@property (nonatomic, assign) NSUInteger sampleCount;
@property (nonatomic, assign) MTLPixelFormat colorPixelFormat;
@property (nonatomic, assign) MTLPixelFormat depthPixelFormat;
@property (nonatomic, assign) MTLPixelFormat stencilPixelFormat;

- (instancetype)initWithRenderPassDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor;

@end

/**
 * MetalContext manages device objects, shader pipelines, font textures, and dynamic vertex/index buffers.
 */
@interface MetalContext : NSObject

@property (nonatomic, strong, nullable) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, strong, nullable) FramebufferDescriptor *framebufferDescriptor;
@property (nonatomic, strong) NSMutableDictionary<FramebufferDescriptor *, id<MTLRenderPipelineState>> *renderPipelineStateCache;
@property (nonatomic, strong, nullable) id<MTLTexture> fontTexture;
@property (nonatomic, strong) NSMutableArray<MetalBuffer *> *bufferCache;
@property (nonatomic, assign) double lastBufferCachePurge;

- (instancetype)init;
- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device;
- (void)makeFontTextureWithDevice:(id<MTLDevice>)device;
- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device;
- (void)enqueueReusableBuffer:(MetalBuffer *)buffer;
- (nullable id<MTLRenderPipelineState>)renderPipelineStateForFrameAndDevice:(id<MTLDevice>)device;
- (nullable id<MTLRenderPipelineState>)_renderPipelineStateForFramebufferDescriptor:(FramebufferDescriptor *)descriptor
                                                                             device:(id<MTLDevice>)device;
- (void)emptyRenderPipelineStateCache;
- (void)setupRenderState:(void *)drawData
           commandBuffer:(id<MTLCommandBuffer>)commandBuffer
          commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder
     renderPipelineState:(id<MTLRenderPipelineState>)renderPipelineState
            vertexBuffer:(id<MTLBuffer>)vertexBuffer
      vertexBufferOffset:(NSUInteger)vertexBufferOffset;

- (void)renderDrawData:(void *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder;

@end

NS_ASSUME_NONNULL_END
