/******************************************************************************
// This source file is part of the OpenSCAD project.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU General Public License, (see "LICENSE").
******************************************************************************/

extern "C" {
  extern long NlLoadLibrary();
  extern const long NlErrorCode = NlLoadLibrary();
}
