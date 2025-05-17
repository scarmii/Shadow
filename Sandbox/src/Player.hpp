#pragma once

#include "Shadow.hpp"

class Player
{
public:
	virtual ~Player() = default;

	virtual void onRender(float aspectRatio) = 0;
};

class GhostyCat : public Player
{
public:
	GhostyCat();
	virtual ~GhostyCat() = default;

	virtual void onRender(float aspectRatio) override;
private:
	Shadow::Ref<Shadow::Texture2D> m_ghosty;
};

class DemonyCat : public Player
{
public:
	DemonyCat();
	virtual ~DemonyCat() = default;

	virtual void onRender(float aspectRatio) override;
private:
	Shadow::Ref<Shadow::Texture2D> m_demony;
};