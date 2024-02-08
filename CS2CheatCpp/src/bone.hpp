#pragma once

enum bones : int
{
	head = 6,
	neck = 5,
	spine = 4,
	spine_1 = 2,
	left_shoulder = 8,
	left_arm = 9,
	left_hand = 11,
	cock = 0,
	right_shoulder = 13,
	right_arm = 14,
	right_hand = 16,
	left_hip = 22,
	left_knee = 23,
	left_feet = 24,
	right_hip = 25,
	right_knee = 26,
	right_feet = 27,
};

struct BoneConnection
{
	int bone1;
	int bone2;

	BoneConnection(int b1, int b2) : bone1(b1), bone2(b2) {}
};

BoneConnection boneConnections[] = {
	BoneConnection(6, 5),
	BoneConnection(5, 4),
	BoneConnection(4, 0),
	BoneConnection(4, 8),
	BoneConnection(8, 9),
	BoneConnection(9, 11),
	BoneConnection(4, 13),
	BoneConnection(13, 14),
	BoneConnection(14, 16),
	BoneConnection(4, 2),
	BoneConnection(0, 22),
	BoneConnection(0, 25),
	BoneConnection(22, 23),
	BoneConnection(23, 24),
	BoneConnection(25, 26),
	BoneConnection(26, 27),
};