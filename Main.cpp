/*	Hankel Haldin
*	Databases Lab 4: ODBC
*	Due: 7/1/2021
*	Create an external interface for the database based on your design from lab 2 and 3.
*/

#include <iostream>
#include <string>
#include <Windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_MSG_LEN 1024
#define ProgramCompleteWithErrors 1
enum Tables { STUDENT, PROFESSOR, DEPARTMENT, PROJECT };

//Pre: A buffer has been allocated for the handle argument
//Post: A sql error message is presented to the user.
void showSQLError(unsigned int handleType, const SQLHANDLE& handle);

//Pre: Environment and connection handle buffers have been declared.
//Post: The environment and connection handle buffers have been allocated the appropriate sql handles.
bool EnvConnIni(SQLHANDLE& EnvHandle, SQLHANDLE& ConnHandle);

//Pre: SQL statement and connection handles have been allocated.
//Post: Memory for statement and connection handles has been deallocated.
void FreeAndDisconnect(SQLHANDLE StmtHandle, SQLHANDLE ConnHandle);

//Pre: A connection handle has been allocated and connection strings specifing the path to a SQL server have been initialized.
//Post: A connection to a SQL server has been established.
bool ConnectToServer(SQLHANDLE ConnHandle, SQLHANDLE& StmtHandle, SQLWCHAR* ConnStr, SQLWCHAR* RetConnStr);

//Pre: none
//Post: Database table selection menu is presented to the screen.
void DisplayTables();

//Pre: tableSelection parameter has been initialized.
//Post: The appropriate database table is selected.
Tables ProcessTableSelection(char tableSelection);

//Pre: none.
//Post: User input query string is converted to a string of SQLWCHARs.
SQLWCHAR* ReadAndConvertQuery();

//Pre: Memory has been allocated for a SQL statement handle and a query represented by a string of SQLWCHARs has been initialized.
//Post: The result set for the GradStudents table is returned.
void CompleteStudentQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query);

//Pre: Memory has been allocated for a SQL statement handle and a query represented by a string of SQLWCHARs has been initialized.
//Post: The result set for the Professors table is returned.
void CompleteProfessorQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query);

//Pre: Memory has been allocated for a SQL statement handle and a query represented by a string of SQLWCHARs has been initialized.
//Post: The result set for the Departments table is returned.
void CompleteDepartmentQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query);

//Pre: Memory has been allocated for a SQL statement handle and a query represented by a string of SQLWCHARs has been initialized.
//Post: The result set for the Projects table is returned.
void CompleteProjectQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query);

struct StudentRecord
{
	char* studentID;
	char* name;
	char* dob;
	char* major;
};

struct ProfessorRecord {
	char* professorID;
	char* name;
	char* rank;
	char* dob;
	char* specialty;
};

struct DepartmentRecord {
	char* departmentID;
	char* name;
	char* main_office;
};

struct ProjectRecord {
	char* projectID;
	char* sponsor_name;
	char* start_date;
	char* end_date;
	char* budget;
};

int main()
{
	SQLHANDLE sqlconnhandle{ nullptr };	// Connection handle
	SQLHANDLE sqlStmtHandle{ nullptr };	// Statement handle
	SQLHANDLE sqlEnvHandle{ nullptr };		// Environment handle
	std::string connStr = "Driver={SQL Server};SERVER=localhost, 1433, DATABASE=Lab3;Trusted=true;";
	std::string query{ "" };
	SQLWCHAR* sqlConnStr = new SQLWCHAR[connStr.length()];
	for (int i{ 0 }; i < connStr.length(); i++) { sqlConnStr[i] = (SQLWCHAR)connStr[i]; } //Casting each char in string to a SQLWCHAR
	SQLWCHAR* retconstring = new SQLWCHAR[SQL_MSG_LEN];
	SQLWCHAR* sqlQuery;

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

	char tableSelection{ '0' };
	do {
		SQLWCHAR* sqlQuery = { nullptr };

		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlconnhandle, &sqlStmtHandle)) {//Allocate a new statement handle.
			break;
		}

		DisplayTables();
		std::cin >> tableSelection;
		std::cin.ignore(1, '\n'); //Clear buffer for getline use later.
		if (tableSelection == 'q')
			continue;

		switch (ProcessTableSelection(tableSelection)) {

			case STUDENT:
				sqlQuery = ReadAndConvertQuery();
				CompleteStudentQuery(sqlStmtHandle, sqlQuery);
				break;

			case PROFESSOR:
				sqlQuery = ReadAndConvertQuery();
				CompleteProfessorQuery(sqlStmtHandle, sqlQuery);
				break;

			case DEPARTMENT:
				sqlQuery = ReadAndConvertQuery();
				CompleteDepartmentQuery(sqlStmtHandle, sqlQuery);
				break;

			case PROJECT:
				sqlQuery = ReadAndConvertQuery();
				CompleteProjectQuery(sqlStmtHandle, sqlQuery);
				break;
			
			default:
				std::cerr << "\n\n=========================================================================\n";
				std::cerr << "Unrecognized input " << "\"" << tableSelection << "\"" << " -- try again.\n";
				std::cerr << "=========================================================================\n";
		}
		
		} while (tableSelection != 'q');

	FreeAndDisconnect(sqlStmtHandle, sqlconnhandle);
	delete[] sqlConnStr, retconstring, sqlQuery;
	return 0;
}

void showSQLError(unsigned int handleType, const SQLHANDLE& handle)
{
	//My compiler was producing errors when these ids were passed as SQLCHAR arrays so I had to change them to a char array of SQLWCHARS.
	SQLWCHAR* state = new SQLWCHAR[SQL_MSG_LEN];
	SQLWCHAR* msg = new SQLWCHAR[SQL_MSG_LEN];

	if (SQL_SUCCESS == SQLGetDiagRec(handleType, handle, 1, state, NULL, msg, SQL_MSG_LEN, NULL)) {
		cout << "SQL Error message: "; for (int i{ 0 }; i < SQL_MSG_LEN; i++) { cout << (SQLCHAR)msg[i]; }
		cout << "\nSQL state: "; for (int i{ 0 }; i < SQL_MSG_LEN; i++) { cout << (SQLCHAR)state[i]; }
	}
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

void DisplayTables() {
	cout << "Enter the table you would like to perform operations on: \n";
	cout << "1 - GradStudents (PK_sid, name, dob, degree_program)\n";
	cout << "2 - Professors (PK_fid, name, rank, dob, specialty)\n";
	cout << "3 - Departments (PK_did, name, main_office)\n";
	cout << "4 - Projects (PK_pid, sponsor_name, start_date, end_date, budget)\n";
	cout << "q - quit\n";
	cout << "Table number: ";
}

Tables ProcessTableSelection(char tableSelection) {
	switch (tableSelection) {
	case '1':
		return STUDENT;

	case '2':
		return PROFESSOR;

	case '3':
		return DEPARTMENT;

	case '4':
		return PROJECT;
	}
}

SQLWCHAR* ReadAndConvertQuery() {
	std::string userInput{ "" };
	SQLWCHAR* returnString = new SQLWCHAR[userInput.length()];

	cout << "\nNote: Queries to the database should be in the following form: \"select [table] from Lab3.dbo.[table name]\"\n";
	cout << "Enter your query: ";
	std::getline(std::cin, userInput);

	for (int i{ 0 }; i < userInput.length()+1; i++) {
		returnString[i] = (SQLWCHAR)userInput[i];
	}

	return returnString;
}

void CompleteStudentQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query)
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
	StudentRecord StudentData;

	while (SQLFetch(StmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(StmtHandle, 1, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StudentData.studentID = (char*)data;
		cout << "StudentID: " << StudentData.studentID << ", ";

		SQLGetData(StmtHandle, 2, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StudentData.name = (char*)data;
		cout << "Name: " << StudentData.name << ", ";

		SQLGetData(StmtHandle, 3, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StudentData.dob = (char*)data;
		cout << "DOB: " << StudentData.dob << ", ";

		SQLGetData(StmtHandle, 4, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		StudentData.major = (char*)data;
		cout << "Major: " << StudentData.major << endl;
	}
	cout << endl << endl;
}

void CompleteProfessorQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query) {
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
	ProfessorRecord ProfessorData;
	while (SQLFetch(StmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(StmtHandle, 1, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		ProfessorData.professorID = (char*)data;
		cout << "StudentID: " << ProfessorData.professorID << ", ";

		SQLGetData(StmtHandle, 2, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		ProfessorData.name = (char*)data;
		cout << "Name: " << ProfessorData.name << ", ";

		SQLGetData(StmtHandle, 3, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		ProfessorData.rank = (char*)data;
		cout << "Rank: " << ProfessorData.rank;

		SQLGetData(StmtHandle, 4, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		ProfessorData.dob = (char*)data;
		cout << "DOB: " << ProfessorData.dob << ", ";

		SQLGetData(StmtHandle, 5, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		ProfessorData.specialty = (char*)data;
		cout << "Major: " << ProfessorData.specialty << endl;
	}
	cout << endl << endl;
}

void CompleteDepartmentQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query) {
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
	DepartmentRecord DepartmentData;
	while (SQLFetch(StmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(StmtHandle, 1, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		DepartmentData.departmentID = (char*)data;
		cout << "StudentID: " << DepartmentData.departmentID << ", ";

		SQLGetData(StmtHandle, 2, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		DepartmentData.name = (char*)data;
		cout << "Name: " << DepartmentData.name << ", ";

		SQLGetData(StmtHandle, 3, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		DepartmentData.main_office = (char*)data;
		cout << "Rank: " << DepartmentData.main_office;
	}
	cout << endl << endl;
}

void CompleteProjectQuery(SQLHANDLE StmtHandle, SQLWCHAR* Query) {
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
	ProjectRecord projectData;
	while (SQLFetch(StmtHandle) == SQL_SUCCESS)
	{
		SQLGetData(StmtHandle, 1, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		projectData.projectID = (char*)data;
		cout << "StudentID: " << projectData.projectID << ", ";

		SQLGetData(StmtHandle, 2, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		projectData.sponsor_name = (char*)data;
		cout << "Name: " << projectData.sponsor_name << ", ";

		SQLGetData(StmtHandle, 3, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		projectData.start_date = (char*)data;
		cout << "Rank: " << projectData.start_date;

		SQLGetData(StmtHandle, 4, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		projectData.end_date = (char*)data;
		cout << "Rank: " << projectData.end_date;

		SQLGetData(StmtHandle, 5, SQL_CHAR, data, SQL_RESULT_LEN, &ptrSqlData);
		projectData.budget = (char*)data;
		cout << "Rank: " << projectData.budget;
	}
	cout << endl << endl;
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