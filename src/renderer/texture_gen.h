#pragma once

void RendererGenColorAtlas(void* dst, u32 dimension) {
	DASSERT(dimension >= DefaultColors_Count*3);
	u32* pixel = (u32*)dst;

	for (u32 y = 0; y < 2; y++){
		*pixel++ = DefaultColors_Red;
		*pixel++ = DefaultColors_Red;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Green;
		*pixel++ = DefaultColors_Green;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Blue;
		*pixel++ = DefaultColors_Blue;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Black;
		*pixel++ = DefaultColors_Black;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Magenta;
		*pixel++ = DefaultColors_Magenta;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Yellow;
		*pixel++ = DefaultColors_Yellow;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Grey;
		*pixel++ = DefaultColors_Grey;
		*pixel++ = DefaultColors_White;

		*pixel++ = DefaultColors_Cyan;
		*pixel++ = DefaultColors_Cyan;
		*pixel++ = DefaultColors_White;
		*pixel++ = DefaultColors_White;
		*pixel = DefaultColors_White;
		pixel = (u32*)dst + dimension;
	}
}

void RendererGenDefaultTexture(void* dst, u32 dimension) {
	DASSERT(dimension%4 == 0);
	u32* pixel = (u32*)dst;

	for (u32 y = 0; y < dimension; y++) {
		for (u32 x = 0; x < dimension; x++) {
			*pixel++ = DefaultColors_Magenta;
		}
	}
}

void RendererGenWhiteTexture(void* dst, u32 dimension) {
	DASSERT(dimension%4 == 0);
	u32* pixel = (u32*)dst;
	for (u32 y = 0; y < dimension; y++) {
		for (u32 x = 0; x < dimension; x++) {
			*pixel++ = DefaultColors_White;
		}
	}
}

void RendererGenGradientTexture(void* dst, u32 dimension, Vec3 base_color) {
	DASSERT(dimension%4 == 0);
	u32* pixel = (u32*)dst;
	for (u32 y = 0; y < dimension; y++) {
		for (u32 x = 0; x < dimension; x++) {
			f32 red = Lerp(1.0f, base_color.r, (f32)y/(f32)dimension);
			f32 blue = Lerp(1.0f, base_color.b, (f32)y/(f32)dimension);
			f32 green = Lerp(1.0f, base_color.g, (f32)y/(f32)dimension);
			f32 alpha = 1.0f;
			u8 red_byte = 255.0f * red;
			u8 blue_byte = 255.0f * blue;
			u8 green_byte = 255.0f * green;
			u8 alpha_byte = 255.0f;
			*pixel = (alpha_byte<<24) | (red_byte<<16) | (green_byte<<8) | blue_byte;
			pixel++;
		}
	}
}

