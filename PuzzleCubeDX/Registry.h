// entity registry
#pragma once
#include "Components.h"

#include <bitset>
#include <algorithm>

#include <iostream>


// component pool init
constexpr UINT MaxComponents = 16;
constexpr UINT MaxEntities = 256;									// small project 256 entities is fine

// bitset to quickly identify what components an entity has
typedef std::bitset<MaxComponents> Signature;

class IComponent {
public:
	static UINT nextID;
};

template <typename T>
class Component : public IComponent {
public:
	static UINT getID() {
		static UINT typeID = nextID++;
		return typeID;
	}
};

class IPool {
public:
	virtual ~IPool() = default;
};

// pools created for each type of component. eg, Pool<CTransform> stores all CTransform components of all entities. 
// Accessed by using entity id as index for the compData vector. eg, entity id = 0; compData[0] => gives CTransform of 0th index entity 
template <typename T>
class Pool : public IPool{
public:
	Pool() { 
		compData.resize(MaxEntities);							// reserve comp data for maxentities
	}
	std::vector<T> compData;
};


class Registry {
public:
	Registry() : currentEntity(0), nextEntity(0) {
		signatures.resize(MaxEntities);
	}


	UINT createEntity() {
		currentEntity = nextEntity;
		entities.push_back(nextEntity++);
		return currentEntity;
	}

	template <typename T, typename ...TArgs>
	T& addComponent(UINT entity, TArgs&& ...args) {
		auto compTypeID = Component<T>::getID();

		// resize pool vector to store new component pool data
		if (compTypeID >= compPools.size()) {
			compPools.resize(compTypeID + 1, nullptr);
		}
		
		if (!compPools[compTypeID]) {
			compPools[compTypeID] = std::make_shared<Pool<T>>();
		}

		// get that component's pool
		std::shared_ptr<Pool<T>> compPool = std::static_pointer_cast<Pool<T>>(compPools[compTypeID]);

		T component(std::forward<TArgs>(args)...);
		compPool->compData.size();
		compPool->compData[entity] = component;

		// set signature for entity
		signatures[entity].set(compTypeID);

		auto str = signatures[entity].to_string();
		std::cout << str << std::endl;

		return compPool->compData[entity];
	}
	
	template <typename T>
	T& getComponent(UINT entity) {
		auto compTypeID = Component<T>::getID();
		
		std::shared_ptr<Pool<T>> compPool = std::static_pointer_cast<Pool<T>>(compPools[compTypeID]);

		return static_cast<T&>(compPool->compData[entity]);
	}

	template <typename T>
	bool hasComponent(UINT& entity) {
		// check if entity has that comp from its signature
		auto compTypeID = Component<T>::getID();
		return signatures[entity].test(compTypeID);
	}

	// used by system to generate signatures to then get entities that possess those specific components
	template <typename T>
	UINT getComponentTypeID() {
		return Component<T>::getID();
	}

	std::vector<UINT> getEntitiesFromSignature(Signature& signatureMatch) {
		std::vector<UINT> signatureEntities;
		
		UINT index = 0;
		std::for_each(signatures.begin(), signatures.end(), [&](const auto& signature) {
			auto result = signature & signatureMatch;

			if (result.any())
				signatureEntities.push_back(index);
			index++;
			});

		return signatureEntities;
	}

private:

	std::vector<std::shared_ptr<IPool>> compPools;
	std::vector<Signature> signatures;										//signatures of each entity

	std::vector<UINT> entities;
	UINT currentEntity;
	UINT nextEntity;

};
