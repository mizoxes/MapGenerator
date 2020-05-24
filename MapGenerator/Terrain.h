#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	Vertex() :Position(0, 0, 0), Normal(0, 0, 0) {}
	Vertex(float x, float y, float z, float nx, float ny, float nz) :Position(x, y, z), Normal(nx, ny, nz) {}
	Vertex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm) :Position(pos), Normal(norm) {}
};

class Terrain {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
public:
	Terrain(const std::string &heightmap);
private:
	float getHeight(int x, int z);
	DirectX::XMFLOAT3 calcNormal(int x, int z);
};

