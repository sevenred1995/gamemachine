﻿#ifndef __GMSPRITEGAMEOBJECT_H__
#define __GMSPRITEGAMEOBJECT_H__
#include <gmcommon.h>
#include <gmphysics.h>
#include "gmgameobject.h"
BEGIN_NS

GM_ALIGNED_STRUCT(GMSpriteMovement)
{
	GMSpriteMovement() = default;

	GMSpriteMovement(const glm::vec3& dir, const glm::vec3& rate, GMMovement m)
		: moveDirection(dir)
		, moveRate(rate)
		, movement(m)
	{}

	glm::vec3 moveDirection;
	glm::vec3 moveRate;
	GMMovement movement;
};

GM_PRIVATE_OBJECT(GMSpriteGameObject)
{
	GMfloat radius;

	GMfloat pitchLimitRadius;
	GMPositionState state;
	AlignedVector<GMSpriteMovement> movements;
};

class GMSpriteGameObject : public GMGameObject
{
	DECLARE_PRIVATE_AND_BASE(GMSpriteGameObject, GMGameObject)

public:
	GMSpriteGameObject(GMfloat radius);

public:
	virtual GMGameObjectType getType() { return GMGameObjectType::Sprite; }
	virtual void simulate() override;
	virtual void onAppendingObjectToWorld() override;
	virtual void updateAfterSimulate() override;

public:
	void setJumpSpeed(const glm::vec3& jumpSpeed);
	void setMoveSpeed(const glm::vec3& moveSpeed);

	//! 表示精灵对象执行一个动作。
	/*!
	  精灵对象执行一个动作。动作的效果由本对象一些物理属性合成。
	  \param movement 动作的类型，如跳跃、移动等。
	  \param direction 动作的方向。动作方向所采用的坐标系为精灵朝向的坐标系，并采用右手坐标系。即：精灵面朝z轴正方向，精灵左手为x轴正方向，头顶方向为y轴正方向。
	  \param rate 比率。在计算移动的时位移的折扣。如用手柄时，手柄摇杆有个范围，此时通过比率来决定要位移原本要位移的多少。如果摇杆打到尽头，则可以认为比率为1。
	*/
	void action(GMMovement movement, const glm::vec3& direction = glm::vec3(), const glm::vec3& rate = glm::vec3(1));
	void lookUp(GMfloat degree);
	void lookRight(GMfloat degree);
	void setPitchLimitDegree(GMfloat deg);
	const GMPositionState& getPositionState();

private:
	void sendMoveCommand(const glm::vec3& direction, const glm::vec3& rate);
};


END_NS
#endif