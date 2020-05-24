#include "Terrain.h"
#include "ppm.h"

using namespace std;
using namespace DirectX;

constexpr float MAX_PIXEL_COLOR = 256.0f * 256.0f * 256.0f;
constexpr float MAX_HEIGHT      = 40.0f;

static ppm s_image;

Terrain::Terrain(const std::string& heightmap) {

	s_image.read(heightmap);

	int count = s_image.height * s_image.height;

	vertices.resize(count * 3);
	indices.resize(6 * (s_image.height - 1) * (s_image.height - 1));

	int index = 0;
	for (int i = 0; i < s_image.height; i++) {
		for (int j = 0; j < s_image.height; j++) {
			vertices[index].Position.x = j / (float)(s_image.height - 1) * 800.0f - 400.0f;
			vertices[index].Position.y = getHeight(j, i);
			vertices[index].Position.z = i / (float)(s_image.height - 1) * 800.0f - 400.0f;

			XMFLOAT3 normal = calcNormal(j, i);
			vertices[index].Normal.x = normal.x;
			vertices[index].Normal.y = normal.y;
			vertices[index].Normal.z = normal.z;

			index++;
		}
	}

	index = 0;
	for (int i = 0; i < s_image.height - 1; i++) {
		for (int j = 0; j < s_image.height - 1; j++) {

			int topLeft = (i * s_image.height) + j;
			int topRight = topLeft + 1;
			int bottomLeft = ((i + 1) * s_image.height) + j;
			int bottomRight = bottomLeft + 1;

			indices[index++] = topLeft;
			indices[index++] = bottomLeft;
			indices[index++] = topRight;

			indices[index++] = topRight;
			indices[index++] = bottomLeft;
			indices[index++] = bottomRight;
		}
	}
}

float Terrain::getHeight(int x, int z) {
	if (x < 0 || x >= s_image.width ||
		z < 0 || z >= s_image.height) {
		return 0.0f;
	}

	int index = x + z * s_image.width;
	int rgb = s_image.b[index] | (s_image.g[index] << 8) | (s_image.r[index] << 16);

	float height = static_cast<float>(rgb);
	height -= MAX_PIXEL_COLOR / 2.0f;
	height /= MAX_PIXEL_COLOR / 2.0f;
	height *= MAX_HEIGHT;

	return height;
}

XMFLOAT3 Terrain::calcNormal(int x, int z) {
	float heightL = getHeight(x - 1, z);
	float heightR = getHeight(x + 1, z);
	float heightD = getHeight(x, z - 1);
	float heightU = getHeight(x, z + 1);

	return XMFLOAT3( heightL - heightR, 2.0f, heightD - heightU );
}