#include "stereokit.h"
#include "texture.h"

#include "d3d.h"

#pragma warning( disable : 26451 )
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning( default : 26451 )

tex2d_t tex2d_create(const char *id) {
	return (tex2d_t)assets_allocate(asset_type_texture, id);
}
tex2d_t tex2d_find(const char *id) {
	tex2d_t result = (tex2d_t)assets_find(id);
	if (result != nullptr) {
		assets_addref(result->header);
		return result;
	}
	return nullptr;
}
tex2d_t tex2d_create_file(const char *file) {
	tex2d_t result = tex2d_find(file);
	if (result != nullptr)
		return result;

	int      channels = 0;
	int      width    = 0;
	int      height   = 0;
	uint8_t *data     = stbi_load(file, &width, &height, &channels, 4);

	if (data == nullptr) {
		return nullptr;
	}
	result = tex2d_create(file);

	tex2d_set_colors(result, width, height, data);
	free(data);

	return result;
}
tex2d_t tex2d_create_mem(const char *id, void *data, size_t data_size) {
	tex2d_t result = tex2d_find(id);
	if (result != nullptr)
		return result;

	int      channels = 0;
	int      width    = 0;
	int      height   = 0;
	uint8_t *col_data =  stbi_load_from_memory((stbi_uc*)data, data_size, &width, &height, &channels, 4);

	if (col_data == nullptr) {
		return nullptr;
	}
	result = tex2d_create(id);

	tex2d_set_colors(result, width, height, col_data);
	free(col_data);

	return result;
}

void tex2d_release(tex2d_t texture) {
	assets_releaseref(texture->header);
}
void tex2d_destroy(tex2d_t tex) {
	if (tex->resource != nullptr) tex->resource->Release();
	if (tex->texture  != nullptr) tex->texture ->Release();
	*tex = {};
}

void tex2d_set_colors(tex2d_t texture, int32_t width, int32_t height, void *data, tex_format_ data_format) {
	if (texture->width != width || texture->height != height || (texture->resource != nullptr && texture->can_write == false)) {
		if (texture->resource != nullptr) {
			texture->resource->Release();
			texture->can_write = true;
		}

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width            = width;
		desc.Height           = height;
		desc.MipLevels        = 1;
		desc.ArraySize        = 1;
		desc.SampleDesc.Count = 1;
		desc.Format           = tex2d_get_native_format(data_format);
		desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
		desc.Usage            = texture->can_write ? D3D11_USAGE_DYNAMIC    : D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags   = texture->can_write ? D3D11_CPU_ACCESS_WRITE : 0;

		D3D11_SUBRESOURCE_DATA tex_mem;
		tex_mem.pSysMem     = data;
		tex_mem.SysMemPitch = tex2d_format_size(data_format) * width;

		if (FAILED(d3d_device->CreateTexture2D(&desc, &tex_mem, &texture->texture))) {
			log_write(log_error, "Create texture error!");
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC res_desc = {};
		res_desc.Format              = desc.Format;
		res_desc.Texture2D.MipLevels = desc.MipLevels;
		res_desc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
		d3d_device->CreateShaderResourceView(texture->texture, &res_desc, &texture->resource);

		texture->width  = width;
		texture->height = height;
		return;
	}
	texture->width  = width;
	texture->height = height;

	D3D11_MAPPED_SUBRESOURCE tex_mem = {};
	if (FAILED(d3d_context->Map(texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &tex_mem))) {
		log_write(log_error, "Failed mapping a texture");
		return;
	}

	size_t   color_size = tex2d_format_size(data_format);
	uint8_t *dest_line  = (uint8_t*)tex_mem.pData;
	uint8_t *src_line   = (uint8_t*)data;
	for (int i = 0; i < height; i++) {
		memcpy(dest_line, src_line, (size_t)width*color_size);
		dest_line += tex_mem.RowPitch;
		src_line  += width * color_size;
	}
	d3d_context->Unmap(texture->texture, 0);
}

void tex2d_set_active(tex2d_t texture, int slot) {
	if (texture != nullptr)
		d3d_context->PSSetShaderResources(slot, 1, &texture->resource);
	else
		d3d_context->PSSetShaderResources(slot, 0, nullptr);
}

DXGI_FORMAT tex2d_get_native_format(tex_format_ format) {
	switch (format) {
	case tex_format_rgba32:  return DXGI_FORMAT_R8G8B8A8_UNORM;
	case tex_format_rgba64:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case tex_format_rgba128: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	default: return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

size_t tex2d_format_size(tex_format_ format) {
	switch (format) {
	case tex_format_rgba32:  return sizeof(color32);
	case tex_format_rgba64:  return sizeof(uint16_t)*4;
	case tex_format_rgba128: return sizeof(color128);
	default: return sizeof(color32);
	}
}