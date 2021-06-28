#include <iostream>
#include <Windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_MSG_LEN 1024
#define ProgramCompleteWithErrors 1

void showSQLError(unsigned int handleType, const SQLHANDLE& handle);
bool EnvConnIni(SQLHANDLE& EnvHandle, SQLHANDLE& ConnHandle);
void FreeAndDisconnect(SQLHANDLE StmtHandle, SQLHANDLE ConnHandle);
bool ConnectToServer(SQLHANDLE ConnHandle, SQLHANDLE& StmtHandle, SQLWCHAR* ConnStr, SQLWCHAR* RetConnStr);
void CompleteQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query);

struct StudentData
{
	char* StudentID;
	char* Name;
	char* dob;
	char* major;
};

int main()
{
	SQLHANDLE sqlconnhandle = NULL;	// Connection handle
	SQLHANDLE sqlStmtHandle = NULL;	// Statement handle
	SQLHANDLE sqlEnvHandle = NULL;		// Environment handle
	std::string connStr = "Driver={SQL Server};SERVER=localhost, 1433, DATABASE=Lab3;Trusted=true;";
	std::string query = "select * from Lab3.dbo.GradStudents";
	
	SQLWCHAR* sqlConnStr = new SQLWCHAR[connStr.length()];
	for (int i{ 0 }; i < connStr.length(); i++) { sqlConnStr[i] = (SQLWCHAR)connStr[i];} //Casting each char in string to a SQLWCHAR

	SQLWCHAR* retconstring = new SQLWCHAR[SQL_MSG_LEN];

	SQLWCHAR* sqlQuery = new SQLWCHAR[query.length()];
	for (int i{ 0 }; i < query.length()+1; i++) {sqlQuery[i] = query[i];}

	if (!EnvConnIni(sqlEnvHandle, sqlconnhandle))			// Initialize environment and connection handles.
	{
		FreeAndDisconnect(sqlStmtHandle, sqlconnhandle);	// Free handles and disconnect if initialization fails.
		return ProgramCompleteWithErrors;
	}

	if (!ConnectToServer(sqlconnhandle, sqlStmtHandle, sqlConnStr, retconstring))	// Attempts to connect to server.
	{
		FreeAndDisconnect(sqlStmtHandle, sqlconnhandle);	// Free handles and disconnect if initialization fails.
		return ProgramCompleteWithErrors;
	}

	CompleteQuery(sqlStmtHandle, sqlQuery);

	FreeAndDisconnect(sqlStmtHandle, sqlconnhandle);
	delete[] sqlConnStr, retconstring, sqlQuery;
	return 0;
}

void showSQLError(unsigned int handleType, const SQLHANDLE& handle)
{
	//My compiler was producing errors when these ids were passed as SQLCHAR arrays so I had to change them to a char array of SQLWCHARS.
	SQLWCHAR* state = new SQLWCHAR[SQL_MSG_LEN];
	SQLWCHAR* msg = new SQLWCHAR[SQL_MSG_LEN];

	if (SQL_SUCCESS == SQLGetDiagRec(handleType, handle, 1, state, NULL, msg, SQL_MSG_LEN, NULL))
		cout << "SQL Error message: " << *msg << "\nSQL state: " << *state << "." << endl;
	else
		cout << "Error retrieving error information.\n";

	delete[] state, msg;
}

bool EnvConnIni(SQLHANDLE& EnvHandle, SQLHANDLE& ConnHandle)
{
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &EnvHandle))
		return false;

	if (SQL_SUCCESS != SQLSetEnvAttr(EnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		return false;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, EnvHandle, &ConnHandle))
		return false;

	return true;
}

void FreeAndDisconnect(SQLHANDLE StmtHandle, SQLHANDLE ConnHandle)
{
	SQLFreeHandle(SQL_HANDLE_STMT, StmtHandle);
	SQLDisconnect(ConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, ConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, ConnHandle);

	std::cout << "\nPress any key to exit...";
	getchar();
}

bool ConnectToServer(SQLHANDLE ConnHandle, SQLHANDLE& StmtHandle, SQLWCHAR* ConnStr, SQLWCHAR* RetConnStr)
{
	std::cout << "Attempting connection to SQL Server...\n";

	switch (SQLDriverConnect(ConnHandle, NULL, ConnStr, SQL_NTS, RetConnStr, SQL_MSG_LEN, NULL, SQL_DRIVER_NOPROMPT))
	{
	case SQL_SUCCESS:
		std::cout << "Successfully connected to SQL Server\n";
		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, ConnHandle, &StmtHandle))
			return false;
		return true;
		break;

	case SQL_SUCCESS_WITH_INFO:
		std::cout << "Successfully connected to SQL Server\n";
		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, ConnHandle, &StmtHandle))
			return false;
		return true;
		break;

	case SQL_INVALID_HANDLE:
		std::cout << "Could not connect to SQL Server\n";
		return false;
		break;

	case SQL_ERROR:
		std::cout << "Could not connect to SQL Server\n";
		return false;
		break;

	default:
		std::cout << "Could not connect to SQL Server\n";
		return false;
		break;
	}

	return false;
}

void CompleteQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query)
{
	std::cout << "\nExecuting T-SQL query...\n";

	switch (SQLExecDirect(StmtHandle, Query, SQL_NTS))
	{
	case SQL_SUCCESS:
		std::cout << "Success\n";
		break;

	case SQL_SUCCESS_WITH_INFO:
		std::cout << "Success\n";
		break;

	case SQL_NEED_DATA:
		std::cout << "Need data\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	case SQL_STILL_EXECUTING:
		std::cout << "Still executing\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	case SQL_ERROR:
		std::cout << "General error\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	case SQL_NO_DATA:
		std::cout << "No data\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	case SQL_INVALID_HANDLE:
		std::cout << "Invalid handle\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	case SQL_PARAM_DATA_AVAILABLE:
		std::cout << "Param data available\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;

	default:
		std::cout << "Default\n";
		showSQLError(SQL_HANDLE_STMT, StmtHandle);
		break;
	}

	SQLCHAR data[SQL_RESULT_LEN];
	SQLINTEGER ptrSqlData;
	StudentData StuData;		// Here I am just using a single object, however you could use a vector to collect all of the data.

	while (SQLFetch(StmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(StmtHandle, 1, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StuData.StudentID = (char*)data;
		cout << "StudentID: " << StuData.StudentID << ", ";

		SQLGetData(StmtHandle, 2, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StuData.Name = (char*)data;
		cout << "Name: " << StuData.Name << ", ";

		SQLGetData(StmtHandle, 3, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StuData.dob = (char*)data;
		cout << "DOB: " << StuData.dob << ", ";

		SQLGetData(StmtHandle, 4, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StuData.major = (char*)data;
		cout << "Major: " << StuData.major << endl;

		// Etc...
	}
}

// Example query of SQL version, this would need to be modified to fit current code construct.
/*if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLCHAR*)"SELECT @@VERSION", SQL_NTS))
{
	std::cout << "Error querying SQL Server\n";
	goto COMPLETED;
}
else
{
	SQLCHAR sqlVersion[SQL_RESULT_LEN];
	SQLINTEGER ptrSqlVersion;
	while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(sqlStmtHandle, 1, SQL_CHAR, sqlVersion, SQL_RESULT_LEN, &ptrSqlVersion);
		std::cout << "\nQuery Result:\n\n";
		std::cout << sqlVersion << endl;
	}
}
*/