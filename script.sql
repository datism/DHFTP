CREATE DATABASE FileSystem
GO

USE FileSystem
GO

CREATE TABLE Account (
	username	CHAR(10)	NOT NULL,
	password	CHAR(10)	NOT NULL,
	status		BIT			NOT NULL
)
GO