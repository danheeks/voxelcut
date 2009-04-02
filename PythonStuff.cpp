// PythonStuff.cpp
#include "wx/wx.h"
#include "PythonStuff.h"
#include "voxlap5.h"
#include <wx/stdpaths.h>

#if _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif

extern void Repaint();

bool vc_error_called = false;

static PyObject* vc_error(PyObject* self, PyObject* args)
{
	char* str;
	if (!PyArg_ParseTuple(args, "s", &str)) return NULL;
	wxMessageBox(str);
	vc_error_called = true;
	throw str;
	Py_RETURN_NONE;
}

static PyObject* vc_setsphere(PyObject* self, PyObject* args)
{
	//Render a sphere to VXL memory (code is optimized!)
	//   hit: center of sphere
	//hitrad: radius of sphere
	// dacol:  0: insert (additive CSG)
	//        -1: remove (subtractive CSG)
	//To specify color, set vx5.colfunc to the desired procedural texture
	//   before the call
	lpoint3d p;
	long r, dacol;
	if (!PyArg_ParseTuple(args, "iiiii", &p.x, &p.y, &p.z, &r, &dacol)) return NULL;
	
	setsphere(&p, r, dacol);
	updatevxl();

	Py_RETURN_NONE;
}

static PyObject* vc_setcylinder(PyObject* self, PyObject* args)
{
	//    p0: endpoint #1
	//    p1: endpoint #2
	//    cr: radius of cylinder
	// dacol:  0: insert (additive CSG)
	//        -1: remove (subtractive CSG)

	lpoint3d p0, p1;
	long cr, dacol;
	if (!PyArg_ParseTuple(args, "iiiiiiii", &p0.x, &p0.y, &p0.z, &p1.x, &p1.y, &p1.z, &cr, &dacol)) return NULL;
	
	setcylinder(&p0, &p1, cr, dacol, 0);
	updatevxl();

	Py_RETURN_NONE;
}

static PyObject* vc_setrect(PyObject* self, PyObject* args)
{
	//Render a box to VXL memory (code is optimized!)
	//   hit: box corner #1
	//  hit2: box corner #2
	// dacol:  0: insert (additive CSG)
	//        -1: remove (subtractive CSG)

	lpoint3d p0, p1;
	long dacol;
	if (!PyArg_ParseTuple(args, "iiiiiii", &p0.x, &p0.y, &p0.z, &p1.x, &p1.y, &p1.z, &dacol)) return NULL;
	
	setrect(&p0, &p1, dacol);

	Py_RETURN_NONE;
}

static PyObject* vc_repaint(PyObject* self, PyObject* args)
{
	Repaint();

	Py_RETURN_NONE;
}

static PyMethodDef VCMethods[] = {
    {"error", vc_error, METH_VARARGS, ""},
    {"setsphere", vc_setsphere, METH_VARARGS, ""},
    {"setcylinder", vc_setcylinder, METH_VARARGS, ""},
    {"setrect", vc_setrect, METH_VARARGS, ""},
    {"repaint", vc_repaint, METH_VARARGS, ""},
   {NULL, NULL, 0, NULL}
};

static void call_redirect_errors(bool flush = false)
{
	const char* filename = "redir";
	const char* function = flush ? "redirflushfn" : "redirfn";

	PyObject *pName = PyString_FromString(filename);
	PyObject *pModule = PyImport_Import(pName);

    if (pModule != NULL) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, function);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(0);
			PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                Py_DECREF(pValue);
            }
        }

		Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
}

// call a given file
bool call_file(const char* filename)
{
	PyImport_ImportModule(filename);

	if (PyErr_Occurred())
	{
		PyErr_Print();
		return false;
	}

	return true;
}

static wxString GetExeFolder()
{
	wxStandardPaths sp;
	wxString exepath = sp.GetExecutablePath();
	int last_fs = exepath.Find('/', true);
	int last_bs = exepath.Find('\\', true);
	wxString exedir;
	if(last_fs > last_bs){
		exedir = exepath.Truncate(last_fs);
	}
	else{
		exedir = exepath.Truncate(last_bs);
	}

	return exedir;
}

void VoxelCutRunScript()
{
	try{
		::wxSetWorkingDirectory(GetExeFolder());

		Py_Initialize();
		Py_InitModule("vc", VCMethods);

		// redirect stderr
		call_redirect_errors();

		// call the python file
		bool success = call_file("run");

		// flush the error file
		call_redirect_errors(true);

		// display the errors
		if(!success)
		{
			FILE* fp = fopen("error.log", "r");
			std::string error_str;
			int i = 0;
			while(!(feof(fp))){
				char str[1024] = "";
				fgets(str, 1024, fp);
				if(i)error_str.append("\n");
				error_str.append(str);
				i++;
			}
			wxMessageBox(error_str.c_str());
		}

		Py_Finalize();

	}

	catch(...)
	{
		if(vc_error_called)
		{
			vc_error_called = false;
		}
		else
		{
			wxMessageBox("Error while running script!");
			if(PyErr_Occurred())
				PyErr_Print();
		}
		Py_Finalize();
	}

}
