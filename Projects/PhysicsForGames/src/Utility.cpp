#include <cstdio>
#include "gl_core_4_4.h"

bool LoadShaderType(char* filename, GLenum shaderType, unsigned int* output)
{
    //we want to be able to return if we succeded
    bool succeeded = true;

    //open the shader file
    FILE* shaderFile = fopen(filename, "r");

    //did it open successfully 
    if (shaderFile == 0)
        succeeded = false;
    else
    {
        //find out how long the file is
        fseek(shaderFile, 0, SEEK_END);
        int shaderFileLength = ftell(shaderFile);
        fseek(shaderFile, 0, SEEK_SET);
        //allocate enough space for the file
        char* shaderSource = new char[shaderFileLength];
        //read the file and update the length to be accurate
        shaderFileLength = fread(shaderSource, 1, shaderFileLength, shaderFile);

        //create the shader based on the type that got passed in
        unsigned int shaderHandle = glCreateShader(shaderType);
        //compile the shader
        glShaderSource(shaderHandle, 1, &shaderSource, &shaderFileLength);
        glCompileShader(shaderHandle);

        //chech the shader for errors
        int success = GL_FALSE;
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            int logLength = 0;
            glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);
            char* log = new char[logLength];
            glGetShaderInfoLog(shaderHandle, logLength, NULL, log);
            printf("%s\n", log);
            delete[] log;
            succeeded = false;
        }
        //only give the result to the caller if we succeeded
        if (succeeded)
            *output = shaderHandle;

        //clean up the stuff we allocated
        delete[] shaderSource;
        fclose(shaderFile);
    }

    return succeeded;
}

bool LoadShader(char* vertexFilename, char* geometryFilename, char* fragmentFilename, GLuint* result)
{
	bool succeeded = true;
	
	*result = glCreateProgram();
	
	unsigned int vertexShader;
	
	if (LoadShaderType(vertexFilename, GL_VERTEX_SHADER, &vertexShader))
	{
		glAttachShader(*result, vertexShader);
		glDeleteShader(vertexShader);
	}
	else
	    printf("FAILED TO LOAD VERTEX SHADER\n");
	
	if (geometryFilename != nullptr)
	{
		unsigned int geometryShader;
		if (LoadShaderType(geometryFilename, GL_GEOMETRY_SHADER, &geometryShader))
		{
			glAttachShader(*result, geometryShader);
			glDeleteShader(geometryShader);
		}
		else
			printf("FAILED TO LOAD GEOMETRY SHADER\n");
	}
	if (fragmentFilename != nullptr)
	{
		unsigned int fragmentShader;
		if (LoadShaderType(fragmentFilename, GL_FRAGMENT_SHADER, &fragmentShader))
		{
			glAttachShader(*result, fragmentShader);
			glDeleteShader(fragmentShader);
		}
		else
			printf("FAILED TO LOAD FRAGMENT SHADER\n");
	}
	
	glLinkProgram(*result);
	
	GLint success;
	glGetProgramiv(*result, GL_LINK_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint logLength;
		glGetProgramiv(*result, GL_INFO_LOG_LENGTH, &logLength);
		char* log = new char[logLength];
		glGetProgramInfoLog(*result, logLength, 0, log);
		
		printf("ERROR: STUFF DONE SCREWED UP IN UR SHADER BUDDY!\n\n");
		printf("%s", log);
		
		delete[] log;
		succeeded = false;
	}
	
	return succeeded;
}