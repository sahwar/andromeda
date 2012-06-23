/*
 * Copyright (C) 2012, Glad Deschrijver <glad.deschrijver@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pdfviewplugin.h"
#include "pdfvieweditor.h"

#include <QtCore/QtPlugin>
#include <guisystem/editormanager.h>

using namespace GuiSystem;
//using namespace PdfView;

PdfViewPlugin::PdfViewPlugin() :
    ExtensionSystem::IPlugin()
{
}

bool PdfViewPlugin::initialize()
{
    PdfViewEditorFactory *f = new PdfViewEditorFactory(this);
    EditorManager::instance()->addFactory(f);

    return true;
}

Q_EXPORT_PLUGIN(PdfViewPlugin)
