
*************
C style guide
*************

The C style guide for this project heavily borrows from the `NetBSD source code
style guide`_.

.. _NetBSD source code style guide: https://cvsweb.netbsd.org/bsdweb.cgi/src/share/misc/style?rev=1.79;content-type=text%2Fplain

Editor/IDE
==========

To follow the style guide, the editor or IDE settings will have to be adjusted.
Instead of trying to do it manually, it may be possible to use any of these
automatic configuration strategies.

EditorConfig
------------

`EditorConfig`_ is a file format that supported editors can use to automatically
change settings to maintain a consistent coding style. The website
shows which software has native support for the format and which requires a
plugin.

If the CompuGuessr repository is opened in an editor with support for the
EditorConfig file format, the settings written in the ``.editorconfig`` file
should apply automatically.

.. _EditorConfig: https://editorconfig.org/

EditorConfig for Vim
^^^^^^^^^^^^^^^^^^^^

Vim versions 9.0.1799 or newer and Neovim versions 0.9 or newer might come with
the EditorConfig plugin already installed. But, it depends on the platform. For
example, the ``app-text/editorconfig-vim`` package needs to be installed
separately on Gentoo.

After the plugin is confirmed to be installed, it must be enabled with the
following line somewhere in Vim's configuration.

.. code-block::
    :caption: Enabling the EditorConfig plugin

    let g:editorconfig = 1

EditorConfig for Visual Studio Code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The official EditorConfig plugin (``EditorConfig.EditorConfig``) can be
installed from the Visual Studio Marketplace and Open VSX Registry.

Vim
---

Configuration
^^^^^^^^^^^^^

As an alternative to EditorConfig, the ``.vimrc`` file at the root of the
project can apply the appropriate settings if it's sourced or copied into a file
that's sourced as part of Vim's configuration.

Visual Studio Code
------------------

Configuration
^^^^^^^^^^^^^

As an alternative to EditorConfig, the ``.vscode/settings.json`` file will
automatically apply the correct settings when the repository is opened.

Plugins
^^^^^^^

Besides the EditorConfig plugin, some other recommended plugins include:

* clangd (``llvm-vs-code-extensions.vscode-clangd``)
* Ctags Companion (``gediminaszlatkus.ctags-companion``)
* Explicit Folding (``zokugun.explicit-folding``)
* Rewrap (``stkb.rewrap``)

All files
=========

Comments
--------

.. code-block:: c
    :caption: Examples of comments

    /* Single-line comments */

    /*
     * Important single-line comments
     */

    /*
     * Multi-line comments should be written
     * like real paragraphs.
     */

Indentation
-----------

Tab width
^^^^^^^^^

**Do** use a tab width of 8 spaces.

**Don't** use spaces for indentation.

Line width
----------

**Do** maintain a line width of 80 characters.

Header files
============

The order of the contents of a header file should follow this:

#. Copyright template
#. Documentation template
#. Include guard start
#. Include files
#. Macros
#. Forward declarations for structs/unions
#. Public struct/union definitions
#. Typedefs
#. Extern declarations
#. Include guard end

Include guards
--------------

**Do** have an include guard (to protect against multiple inclusion).

.. code-block:: c
    :caption: Example of an include guard

    /* header.h */

    #ifndef HEADER_H
    #define HEADER_H

    /* ... */

    #endif  /* !HEADER_H */

**Don't** use the ``#pragma once`` directive.

Macros
------

Naming
^^^^^^

**Do** capitalize the names of macros.

Arguments
^^^^^^^^^

**Do** place every instance of an argument in the macro body within parentheses.

Multiple statements
^^^^^^^^^^^^^^^^^^^

**Do** place multiple statements within a ``do { ... } while (0)`` block.

.. code-block:: c
    :caption: Example of a multi-statement macro

    #define MACRO(v, w, x, y)   \
    do {                        \
    	v = (x) + (y);          \
    	w = (x) * (y);          \
    } while (0)

Structs/unions
--------------

Forward declarations
^^^^^^^^^^^^^^^^^^^^

**Do** use forward declarations for structs/unions if the definitions aren't
needed.

Public definitions
^^^^^^^^^^^^^^^^^^

**Do** define public structs/unions only if they're user allocated or otherwise
exposed to users for a good reason.

See :ref:`typedefs`.

.. _typedefs:

Typedefs
--------

**Do** use typedefs for:

* Function/function pointer types (although non-pointer is preferred)
* Arithmetic types
* Types that may differ by platform

**Don't** use typedefs for:

* Struct/union definitions
* Pointers

Extern declarations
-------------------

**Do** put extern declarations in header files only. But, an assembly subroutine
in an adjacent file that's used in a single C source file can have its extern
declaration inside that C source file.

**Don't** use the ``extern`` keyword with function declarations because it's
redundant.

Source files
============

The order of the contents of a source file should follow this:

#. Copyright template
#. Documentation template
#. Kernel include files
#. Network include files
#. ``/usr`` include files
#. Local include files
#. Static function declarations
#. Macros
#. Enums
#. Struct definitions
#. Function definitions

Includes
--------

**Do** include a header if a specific typedef, macro, or struct/union
definition is needed.

.. code-block:: c
    :caption: Examples of when a header file needs to be included

**Don't** include a header if a struct/union definition isn't needed. If only a
pointer to a struct/union is used, then put a forward declaration somewhere in
the source file instead.

.. code-block:: c
    :caption: Example of a forward declaration for a struct/union

Transient includes
^^^^^^^^^^^^^^^^^^

**Do** include header files directly that contain the specific typedef, macro,
or struct/union definition.

**Don't** rely on a header file including another header file.

Function definitions
--------------------

.. code-block:: c
    :caption: Example of a function definition
