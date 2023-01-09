#pragma once
#include <iostream>
#include <string>
#include <map>

typedef void *(*create_fun)();

class OBJFactory
{
public:
    ~OBJFactory() {}

    void *getObjectByName(std::string name)
    {
        std::map<std::string, create_fun>::iterator it = m_name_map.find(name);
        if (it == m_name_map.end())
        {
            return NULL;
        }

        create_fun fun = it->second;
        if (!fun)
        {
            fprintf(stderr, "[%s] doesn`t REGISTER\n", name.c_str());
            // stderr << "[" << name << "]" << " doesn`t REGISTER" << std::endl;
            return NULL;
        }
        void *obj = fun();
        return obj;
    }

    void *getObjectByID(int clsid)
    {
        std::map<int, create_fun>::iterator it = m_id_map.find(clsid);
        if (it == m_id_map.end())
        {
            return NULL;
        }

        create_fun fun = it->second;
        if (!fun)
        {
            fprintf(stderr, "[%d] doesn`t REGISTER\n", clsid);
            // stderr << "[" << name << "]" << " doesn`t REGISTER" << std::endl;
            return NULL;
        }
        void *obj = fun();
        return obj;
    }

    bool contain(std::string name)
    {
        std::map<std::string, create_fun>::iterator it = m_name_map.find(name);
        if (it == m_name_map.end())
        {
            return false;
        }
        return true;
    }

    bool contain(int clsid)
    {
        std::map<int, create_fun>::iterator it = m_id_map.find(clsid);
        if (it == m_id_map.end())
        {
            return false;
        }
        return true;
    }

    void print_obj()
    {
        auto it = m_name_map.begin();
        auto itid = m_id_map.begin();
        for (size_t i = 0; i < m_name_map.size(); i++)
        {
            printf("\033[1;30;33m" "%s-%d" "\033[0m" "\n", it->first.c_str(), itid->first);
            it++;
            itid++;
        }
    }

    //????????????????????????
    void registClass(int clsid, std::string name, create_fun fun)
    {
        extern std::map<std::string, int> ModelTypeTable;
        m_name_map[name] = fun;
        m_id_map[clsid] = fun;
        ModelTypeTable[name] = clsid;
    }

    //??????
    static OBJFactory &getInstance()
    {
        static OBJFactory fac;
        return fac;
    }

private:
    OBJFactory() {} //???
    std::map<std::string, create_fun> m_name_map;
    std::map<int, create_fun> m_id_map;
};

class RegisterAction
{
public:
    RegisterAction(int clsid, std::string className, create_fun ptrCreateFn)
    {
        OBJFactory::getInstance().registClass(clsid, className, ptrCreateFn);
    }
};

#define REGISTER(clsid, className)                       \
    static void *objectCreator_##className()             \
    {                                                    \
        return new className;                            \
    }                                                    \
    static RegisterAction g_creatorRegister_##className( \
        clsid, #clsid, (create_fun)objectCreator_##className);