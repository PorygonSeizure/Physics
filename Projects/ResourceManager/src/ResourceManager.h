#ifndef	_RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include <map>
#include <string>

template<typename T>
class ResourceManager
{
public:
	static T* Load(const char* filename)
	{
		auto iter = m_resources.find(filename);
		if (iter == m_resources.end())
		{
			T* resource = new T(filename);
			m_resources[filename] = resource;
			return resource;
		}
		
		return iter->second;
	}

	static unsigned int CountLoadedResources()
	{
		return m_resources.size();
	}

	static void FreeAll()
	{
		for (auto iter = m_resources.begin(); iter != m_resources.end(); iter++)
			delete iter->second;
		m_resources.clear();
	}

private:
	static std::map<std::string, T*> m_resources;
};

template<typename T>
std::map<std::string, T*> ResourceManager<T>::m_resources;

#endif