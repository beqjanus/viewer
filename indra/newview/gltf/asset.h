#pragma once

/**
 * @file asset.h
 * @brief LL GLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llvertexbuffer.h"
#include "llvolumeoctree.h"
#include "../lltinygltfhelper.h"
#include "accessor.h"
#include "primitive.h"
#include "animation.h"
#include "boost/json.hpp"

extern F32SecondsImplicit		gFrameTimeSeconds;


// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        using Value = boost::json::value;

        constexpr S32 LINEAR = 9729;
        constexpr S32 NEAREST = 9728;
        constexpr S32 NEAREST_MIPMAP_NEAREST = 9984;
        constexpr S32 LINEAR_MIPMAP_NEAREST = 9985;
        constexpr S32 NEAREST_MIPMAP_LINEAR = 9986;
        constexpr S32 LINEAR_MIPMAP_LINEAR = 9987;
        constexpr S32 CLAMP_TO_EDGE = 33071;
        constexpr S32 MIRRORED_REPEAT = 33648;
        constexpr S32 REPEAT = 10497;

        class Asset;

        class Material
        {
        public:
            class TextureInfo
            {
            public:
                S32 mIndex = INVALID_INDEX;
                S32 mTexCoord = 0;

                bool operator==(const TextureInfo& rhs) const;
                bool operator!=(const TextureInfo& rhs) const;
                
                const TextureInfo& operator=(const tinygltf::TextureInfo& src);
                const TextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class NormalTextureInfo : public TextureInfo
            {
            public:
                F32 mScale = 1.0f;

                const NormalTextureInfo& operator=(const tinygltf::NormalTextureInfo& src);
                const NormalTextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class OcclusionTextureInfo : public TextureInfo
            {
            public:
                F32 mStrength = 1.0f;

                const OcclusionTextureInfo& operator=(const tinygltf::OcclusionTextureInfo& src);
                const OcclusionTextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class PbrMetallicRoughness
            {
            public:
                glh::vec4f mBaseColorFactor = glh::vec4f(1.f,1.f,1.f,1.f);
                TextureInfo mBaseColorTexture;
                F32 mMetallicFactor = 1.0f;
                F32 mRoughnessFactor = 1.0f;
                TextureInfo mMetallicRoughnessTexture;

                bool operator==(const PbrMetallicRoughness& rhs) const;
                bool operator!=(const PbrMetallicRoughness& rhs) const;
                const PbrMetallicRoughness& operator=(const tinygltf::PbrMetallicRoughness& src);
                const PbrMetallicRoughness& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };


            // use LLFetchedGLTFMaterial for now, but eventually we'll want to use
            // a more flexible GLTF material implementation instead of the fixed packing
            // version we use for sharable GLTF material assets
            LLPointer<LLFetchedGLTFMaterial> mMaterial;
            PbrMetallicRoughness mPbrMetallicRoughness;
            NormalTextureInfo mNormalTexture;
            OcclusionTextureInfo mOcclusionTexture;
            TextureInfo mEmissiveTexture;


            std::string mName;
            glh::vec3f mEmissiveFactor = glh::vec3f(0.f, 0.f, 0.f);
            std::string mAlphaMode = "OPAQUE";
            F32 mAlphaCutoff = 0.5f;
            bool mDoubleSided = false;

            // bind for rendering
            void bind(Asset& asset);
            const Material& operator=(const tinygltf::Material& src);
            const Material& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
            
            void allocateGLResources(Asset& asset);
        };

        class Mesh
        {
        public:
            std::vector<Primitive> mPrimitives;
            std::vector<double> mWeights;
            std::string mName;

            const Mesh& operator=(const tinygltf::Mesh& src);
            const Mesh& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
            
            void allocateGLResources(Asset& asset);
        };

        class Node
        {
        public:
            Node() { mMatrix.setIdentity(); }

            LLMatrix4a mMatrix; //local transform
            LLMatrix4a mRenderMatrix; //transform for rendering
            LLMatrix4a mAssetMatrix; //transform from local to asset space
            LLMatrix4a mAssetMatrixInv; //transform from asset to local space

            glh::vec3f mTranslation;
            glh::quaternionf mRotation;
            glh::vec3f mScale = glh::vec3f(1.f,1.f,1.f);

            // if true, mMatrix is valid and up to date
            bool mMatrixValid = false;

            // if true, translation/rotation/scale are valid and up to date
            bool mTRSValid = false;
            
            bool mNeedsApplyMatrix = false;

            std::vector<S32> mChildren;
            S32 mParent = INVALID_INDEX;

            S32 mMesh = INVALID_INDEX;
            S32 mSkin = INVALID_INDEX;

            std::string mName;

            const Node& operator=(const tinygltf::Node& src);
            const Node& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            // Set mRenderMatrix to a transform that can be used for the current render pass
            // modelview -- parent's render matrix
            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);

            // update mAssetMatrix and mAssetMatrixInv
            void updateTransforms(Asset& asset, const LLMatrix4a& parentMatrix);

            // ensure mMatrix is valid -- if mMatrixValid is false and mTRSValid is true, will update mMatrix to match Translation/Rotation/Scale
            void makeMatrixValid();

            // ensure Translation/Rotation/Scale are valid -- if mTRSValid is false and mMatrixValid is true, will update Translation/Rotation/Scale to match mMatrix
            void makeTRSValid();

            // Set rotation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setRotation(const glh::quaternionf& rotation);

            // Set translation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setTranslation(const glh::vec3f& translation);

            // Set scale of this node
            // SIDE EFFECT: invalidates mMatrix
            void setScale(const glh::vec3f& scale);
        };

        class Skin
        {
        public:
            S32 mInverseBindMatrices = INVALID_INDEX;
            S32 mSkeleton = INVALID_INDEX;
            std::vector<S32> mJoints;
            std::string mName;
            std::vector<glh::matrix4f> mInverseBindMatricesData;

            void allocateGLResources(Asset& asset);
            void uploadMatrixPalette(Asset& asset, Node& node);

            const Skin& operator=(const tinygltf::Skin& src);
            const Skin& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Scene
        {
        public:
            std::vector<S32> mNodes;
            std::string mName;

            const Scene& operator=(const tinygltf::Scene& src);
            const Scene& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            void updateTransforms(Asset& asset);
            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);
        };

        class Texture
        {
        public:
            S32 mSampler = INVALID_INDEX;
            S32 mSource = INVALID_INDEX;
            std::string mName;

            const Texture& operator=(const tinygltf::Texture& src);
            const Texture& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Sampler
        {
        public:
            S32 mMagFilter = LINEAR;
            S32 mMinFilter = LINEAR_MIPMAP_LINEAR;
            S32 mWrapS = REPEAT;
            S32 mWrapT = REPEAT;
            std::string mName;

            const Sampler& operator=(const tinygltf::Sampler& src);
            const Sampler& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Image
        {
        public:
            std::string mName;
            std::string mUri;
            std::string mMimeType;

            S32 mBufferView = INVALID_INDEX;

            std::vector<U8> mData;
            S32 mWidth = -1;
            S32 mHeight = -1;
            S32 mComponent = -1;
            S32 mBits = -1;
            S32 mPixelType = -1;

            LLPointer<LLViewerFetchedTexture> mTexture;

            const Image& operator=(const tinygltf::Image& src);
            const Image& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
            
            // save image clear local data, and set uri
            void decompose(Asset& asset, const std::string& filename);

            // erase the buffer view associated with this image
            // free any associated resources
            // preserve only uri and name
            void clearData(Asset& asset);

            void allocateGLResources();
        };

        // C++ representation of a GLTF Asset
        class Asset
        {
        public:

            static const std::string minVersion_default;
            std::vector<Scene> mScenes;
            std::vector<Node> mNodes;
            std::vector<Mesh> mMeshes;
            std::vector<Material> mMaterials;
            std::vector<Buffer> mBuffers;
            std::vector<BufferView> mBufferViews;
            std::vector<Texture> mTextures;
            std::vector<Sampler> mSamplers;
            std::vector<Image> mImages;
            std::vector<Accessor> mAccessors;
            std::vector<Animation> mAnimations;
            std::vector<Skin> mSkins;

            std::string mVersion;
            std::string mGenerator;
            std::string mMinVersion;
            std::string mCopyright;

            S32 mDefaultScene = INVALID_INDEX;
            Value mExtras;

            // the last time update() was called according to gFrameTimeSeconds
            F32 mLastUpdateTime = gFrameTimeSeconds;

            // prepare the asset for rendering
            void allocateGLResources(const std::string& filename = "", const tinygltf::Model& model = tinygltf::Model());
            
            // Called periodically (typically once per frame)
            // Any ongoing work (such as animations) should be handled here
            // NOT guaranteed to be called every frame
            // MAY be called more than once per frame
            // Upon return, all Node Matrix transforms should be up to date
            void update();

            // update asset-to-node and node-to-asset transforms
            void updateTransforms();

            // update node render transforms
            void updateRenderTransforms(const LLMatrix4a& modelview);
            
            void render(bool opaque, bool rigged = false);
            void renderOpaque();
            void renderTransparent();

            // return the index of the node that the line segment intersects with, or -1 if no hit
            // input and output values must be in this asset's local coordinate frame
            S32 lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
                LLVector4a* intersection = nullptr,         // return the intersection point
                LLVector2* tex_coord = nullptr,            // return the texture coordinates of the intersection point
                LLVector4a* normal = nullptr,               // return the surface normal at the intersection point
                LLVector4a* tangent = nullptr,             // return the surface tangent at the intersection point
                S32* primitive_hitp = nullptr           // return the index of the primitive that was hit
            );
            
            Asset() = default;
            Asset(const tinygltf::Model& src);
            Asset(const Value& src);

            const Asset& operator=(const tinygltf::Model& src);
            const Asset& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            // save the asset to a tinygltf model
            void save(tinygltf::Model& dst);

            // decompose the asset to the given .gltf file
            void decompose(const std::string& filename);

            // remove the bufferview at the given index
            // updates all bufferview indices in this Asset as needed
            void eraseBufferView(S32 bufferView);
        };
    }
}