#pragma once

#include "Component.h"

struct TransformParams
{
	Matrix matWorld;
	Matrix matView;
	Matrix matProjection;
	Matrix matWV;
	Matrix matWVP;
};

class Transform : public Component
{
public:
	Transform();
	virtual ~Transform();

	virtual void	FinalUpdate();
	void			PushData();

	float	GetBoundingSphereRadius() { return max(max(_localScale.x, _localScale.y), _localScale.z); }

public:

	const Matrix&	GetLocalToWorldMatrix() { return _matWorld; }
	const Vec3&		GetWorldPosition() { return _matWorld.Translation(); }

	Vec3	GetRight() { return _matWorld.Right(); }
	Vec3	GetUp() { return _matWorld.Up(); }
	Vec3	GetLook() { return _matWorld.Backward(); }

	const Vec3&	GetLocalPosition() { return _localPosition; }
	const Vec3&	GetLocalRotation() { return _localRotation; }
	const Vec3&	GetLocalScale() { return _localScale; }

	weak_ptr<Transform> GetParent() { return _parent; }

public:

	void	SetLocalPosition(const Vec3& position) { _localPosition = position; }
	void	SetLocalRotation(const Vec3& rotation) { _localRotation = rotation; }
	void	SetLocalScale(const Vec3& scale) { _localScale = scale; }

	void	SetParent(shared_ptr<Transform> parent) { _parent = parent; }

private:
	Vec3	_localPosition = {};
	Vec3	_localRotation = {};
	Vec3	_localScale = { 1.f, 1.f, 1.f };

	Matrix	_matLocal = {};
	Matrix	_matWorld = {};

	weak_ptr<Transform>	_parent;
};

