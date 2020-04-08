#include "renik_chain.h"

void RenIKChain::set_chain(Skeleton *skeleton, BoneId p_root_bone, BoneId p_leaf_bone) {
	root_bone = p_root_bone < p_leaf_bone || p_leaf_bone < 0 ? p_root_bone : p_leaf_bone;
	leaf_bone = p_root_bone < p_leaf_bone || p_root_bone < 0 ? p_leaf_bone : p_root_bone;
	joints.clear();
	total_length = 0;
	if (skeleton && root_bone >= 0 && leaf_bone >= 0 && root_bone < skeleton->get_bone_count() && leaf_bone < skeleton->get_bone_count()) {
		BoneId bone = skeleton->get_bone_parent(leaf_bone);
		//generate the chain of bones
		Vector<BoneId> chain;
		float last_length;
		while (bone != root_bone) {
			last_length = skeleton->get_bone_rest(bone).origin.length();
			total_length += last_length;
			if (bone < 0) { //invalid chain
				total_length = 0;
				return;
			}
			chain.push_back(bone);
			bone = skeleton->get_bone_parent(bone);
		}
		total_length -= last_length;
		total_length += skeleton->get_bone_rest(leaf_bone).origin.length();

		if (total_length <= 0) { //invalid chain
			total_length = 0;
			return;
		}

		Basis totalRotation;
		float progress = 0;
		//flip the order and figure out the relative distances of these joints
		for (int i = chain.size() - 2; i >= 0; i--) {//skips the last joint because we're only doing joints we can move
			Joint j;
			j.id = chain[i];
			Transform boneTransform = skeleton->get_bone_rest(j.id);
			j.relative_prev = totalRotation.xform_inv(boneTransform.origin);
			j.prev_distance = j.relative_prev.length();

			//calc influences
			progress += j.prev_distance;
			float percentage = (progress / total_length);
			float effectiveRootInfluence = root_influence <= 0 || percentage >= root_influence ? 0 : (percentage - root_influence) / -root_influence;
			float effectiveLeafInfluence = leaf_influence <= 0 || percentage <= leaf_influence ? 0 : (percentage - (total_length - leaf_influence)) / leaf_influence;
			float effectiveTwistInfluence = twist_start >= 1 || twist_influence <= 0 || percentage >= twist_start ? 0 : (percentage - twist_start) * (twist_influence / (1 - twist_start));
			j.root_influence = effectiveRootInfluence;
			j.leaf_influence = effectiveLeafInfluence;
			j.twist_influence = effectiveTwistInfluence;

			if (!joints.empty()) {
				joints[joints.size() - 1].relative_next = -j.relative_prev;
				joints[joints.size() - 1].next_distance = j.prev_distance;
			}
			joints.push_back(j);
			totalRotation = (totalRotation * boneTransform.basis).orthonormalized();
		}
	}
}

bool RenIKChain::is_valid() {
	return bones.size() > 2 && !joints.empty();
}

float RenIKChain::get_total_length() {
	return total_length;
}

Vector<Vector3> RenIKChain::get_joints() {
	return joints;
}