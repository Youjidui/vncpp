#include "vtPersistence.h"
#include "FilePersistenceContainer.h"


PersistenceContainerPtr PersistenceEngine::createPersistenceContainer(const std::string& dbURI)
{
	const std::string mongodb = "mongodb";
	const std::string file = "file";
	if(strncmp(mongodb.c_str(), dbURI.c_str(), mongodb.length()) == 0)
	{
		//dbClient = std::make_shared<mongocxx::client>(dbURI);
	}
	else if(strncmp(file.c_str(), dbURI.c_str(), file.length()) == 0)
	{
		return std::make_shared<FilePersistenceContainer>(dbURI);
	}
	return PersistenceContainerPtr();
}



