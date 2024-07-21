#pragma once

#include <string>
#include <array>
#include <map>

#include <Singleton.hpp>

#include <openvr_driver.h>

struct Controller {
	uint32_t m_objectId { 999 };
	vr::ETrackedControllerRole m_role { vr::ETrackedControllerRole::TrackedControllerRole_Invalid };
	vr::DriverPose_t m_pose { 0 };
};

struct Component {
	vr::VRInputComponentHandle_t m_orig;
	vr::VRInputComponentHandle_t m_override;
	vr::VRInputComponentHandle_t& getOverride() { return m_override; }
};

class DeviceController {
public:
	explicit DeviceController();
	inline static DeviceController& get() { return Singleton<DeviceController>::get(); }

	bool isHandTrackingEnabled() const { return m_isHandTrackingEnabled; }
	bool areControllersAvailable() const { return m_areControllersAvailable; }

	void UpdateControllerPose(vr::ETrackedControllerRole role, vr::DriverPose_t pose);
	void UpdateControllerId(vr::ETrackedControllerRole role, uint32_t objectId);
	Controller GetController(vr::ETrackedControllerRole role);

	Component& getComponent(int index);
	std::map<int, Component> GetComponents() { return m_components; }
private:
	bool m_isHandTrackingEnabled { false };
	bool m_areControllersAvailable { false };
	std::array<Controller, 2> m_controllers;
	std::map<int, Component> m_components;
};