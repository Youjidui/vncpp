#pragma once

#include <stdint.h>
#include <memory>
#include <functional>
#include <string.h>
#include <string>
#include <vector>

typedef std::vector<std::string> Record;

class TableBase
{
protected:
	Record m_head;	//column names
	Record m_cursor;	//current record
	static Record m_end;		//end()

public:
	virtual ~TableBase(){}
	virtual Record* next() = 0;
public:
    struct iterator
    {
        TableBase* _m;
        Record* _r;

        iterator(TableBase* m, Record* r) : _m(m), _r(r) {}
        iterator(const iterator& i) : _m(i._m), _r(i._r) {}

        iterator& operator++()
        {
            _r = (*_m).next();
            return *this;
        }
        iterator operator++(int)
        {
			iterator i(*this); 
            _r = (*_m).next();
            return i;
        }
        Record& operator*()
        {
            return *_r;
        }
        Record* operator->()
        {
            return _r;
        }
        bool operator==(const iterator& i) const 
        {
            return _r == i._r;
        }
        bool operator!=(const iterator& i) const 
        {
            return _r != i._r;
        }
    };

	virtual iterator begin() = 0;
	virtual iterator end() 
	{
		return iterator(this, &m_end);
	}

	int columnIndex(const std::string& columnName)
	{
		int index = 0;
		for(auto i = m_head.begin(); i != m_head.end(); ++i, ++index)
		{
			if(columnName == *i)
				return index;
		}
		return -1;
	}
};

typedef std::shared_ptr<TableBase> TablePtr;

class PersistenceContainerBase
{
public:
	virtual ~PersistenceContainerBase(){}
	virtual bool connect() = 0;

public:
	// dbName/tableName
	virtual TablePtr operator[](const std::string& resName) = 0;
};

typedef std::shared_ptr<PersistenceContainerBase> PersistenceContainerPtr;

