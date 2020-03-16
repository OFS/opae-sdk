#
# Copyright (c) 2018, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of the Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# Load a Platform Interface Manager JSON database.
#

from __future__ import absolute_import
from __future__ import print_function
import os
import sys
import json


class jsondb(object):
    #
    # The constructor loads a database and returns it as a dictionary in db.
    #
    def __init__(self, fname=None,
                 db_search_path=None, db_category=None,
                 quiet=False):
        self.file_path = None
        self.file_name = None
        self.quiet = quiet

        self.db = {}
        self.db_category = db_category

        if (not fname):
            self.__errorExit("jsondb() expects a database path")

        if (db_category not in ['AFU', 'platform', 'platform-params']):
            self.__errorExit("Illegal db_category ({0})".format(db_category))

        self.__loadDb(fname, db_search_path)

    #
    # Canonicalize and check the database
    #
    def canonicalize(self):
        if (self.db_category == 'platform-params'):
            self.__canonicalizePlatformDefaultsDb()
        else:
            self.__canonicalizeDb()

    #
    # Dump the database (for debugging)
    #
    def dump(self, fname):
        try:
            with open(fname, "w") as f:
                json.dump(self.db, f, indent=4, sort_keys=True)
        except IOError:
            self.__errorExit("Failed to open {0} for writing.".format(fname))

    def __errorExit(self, msg):
        sys.stderr.write("\nError: " + msg + "\n")
        sys.exit(1)

    #
    # Return a dictionary derived from a JSON file, using a search path.
    #
    def __loadDb(self, fname, db_search_path):
        if (os.path.isfile(fname)):
            json_fname = fname
        else:
            # Find the DB in a directory using the search path
            json_fname = None
            for db_dir in db_search_path:
                fn = os.path.join(db_dir, fname + ".json")
                if (os.path.isfile(fn)):
                    json_fname = fn
                    break

            if (not json_fname):
                c = self.db_category
                if (c == 'AFU'):
                    c = 'AFU top-level interface'
                self.__errorExit(
                    "Failed to find {0} JSON file for {1}".format(c, fname))

        if (not self.quiet):
            print(("Loading {0} database: {1}".format(
                self.db_category, json_fname)))

        self.file_path = json_fname
        self.file_name = os.path.splitext(os.path.basename(json_fname))[0]

        db = self.__loadJsonDbWithIncludes(json_fname)

        # Store the file path in the dictionary and in the class
        db['file_path'] = self.file_path
        db['file_name'] = self.file_name

        # First pass canonicalization guarantees that module ports
        # are ready for merging with parents.
        db = self.__canonicalizeStg1Db(db)

        # Does the database have a parent with more data?
        if ('parent' in db):
            if (not self.quiet):
                print("  Loading parent database: {0}".format(db['parent']))

            # Load parents recursively.
            db_parent = jsondb(db['parent'], db_search_path, self.db_category,
                               self.quiet).db
            if (self.db_category == 'platform'):
                db = self.__mergeDbs(db_parent, db, 'module-ports-offered')
            elif (self.db_category == 'AFU'):
                db = self.__mergeDbs(db_parent, db, 'module-ports')
            else:
                self.__errorExit(
                    ("'parent' keys are not supported in {0} " +
                     "databases ({1})").format(self.db_category, json_fname))

        self.db = db

    #
    # Load a JSON file and detect include directives within it, replacing the
    # include with the contents of the included JSON sub-file.
    #
    def __loadJsonDbWithIncludes(self, json_fname, parent_fname=None):
        try:
            f = open(json_fname)
        except IOError:
            if (parent_fname):
                self.__errorExit(
                    "Failed to open file {0}, included by {1}".format(
                        json_fname, parent_fname))
            else:
                self.__errorExit("Failed to open file {0}".format(json_fname))

        db = json.load(f)
        f.close()

        db = self.__replaceJsonIncludes(db, json_fname)

        return db

    #
    # Do a recursive walk of a loaded JSON database, looking for dictionaries
    # with the key "...".  Treat the values of "..." keys as a relative path
    # of another JSON file.  The included JSON file replaces the contents of
    # the "..." dictionary.
    #
    def __replaceJsonIncludes(self, db, json_fname):
        if (not isinstance(db, dict) and not isinstance(db, list)):
            return db

        for k, v in db.items():
            if (k == '...'):
                path = os.path.join(os.path.dirname(json_fname), v)
                return self.__loadJsonDbWithIncludes(path, json_fname)
            elif isinstance(v, dict):
                db[k] = self.__replaceJsonIncludes(v, json_fname)
            elif isinstance(v, list):
                for i, e in enumerate(v):
                    db[k][i] = self.__replaceJsonIncludes(e, json_fname)

        return db

    #
    # Merge parent and child databases by overwriting parent
    # fields with updates from the child.
    #
    # Note: for module-ports and module-ports-offered,
    # the child completely overwrites an entry.  Namely, for
    # AFUs if both the parent and child have a local-memory
    # class then the parent's local-memory descriptor is deleted
    # and replaced with the child's.  For platform databases,
    # the same is true, but for class/interface pairs.
    #
    def __mergeDbs(self, db, db_child, module_port_key):
        # Copy everything from the child that isn't a module ports.
        # Ports are special.  They will be checked by class.
        for k in db_child.keys():
            if (k != module_port_key):
                db[k] = db_child[k]

        if (module_port_key not in db):
            # No parent module ports
            if (module_port_key in db_child):
                db[module_port_key] = db_child[module_port_key]
        elif (module_port_key in db_child):
            # Both databases have module ports.  Overwrite any parent entries
            # with matching classes.
            for k in db_child[module_port_key].keys():
                db[module_port_key][k] = db_child[module_port_key][k]

        return db

    #
    # First canonicalization pass over a database.  This pass runs before
    # parent databases are imported, so many fields may be missing.
    #
    def __canonicalizeStg1Db(self, db):
        if (not isinstance(db, dict)):
            self.__errorExit("{0} interface JSON is not a dictionary!".format(
                self.db_category))

        fname = self.file_path

        # Convert module ports lists to dictionaries.
        for ports_key in ['module-ports', 'module-ports-offered']:
            if (ports_key in db):
                port_dict = dict()

                for port in db[ports_key]:
                    # Module ports must be dictionaries
                    if (not isinstance(port, dict)):
                        self.__errorExit(
                            "{0} in {1} must be dictionaries ({2})".format(
                                ports_key, fname, port))

                    # Check for mandatory keys
                    for key in ['class', 'interface']:
                        if (key not in port):
                            self.__errorExit(
                                "module port {0} is missing {1} in {2}".format(
                                    port, key, fname))

                    # For AFU module-ports the key is just the class, since
                    # classes must be unique.  Platforms may offer more than
                    # one instance of a class, so their keys are
                    # class/instance.
                    k = port['class']
                    if (ports_key == 'module-ports-offered'):
                        k = k + '/' + port['interface']

                    # No duplicate keys allowed!
                    if k in port_dict:
                        self.__errorExit(
                            ("multiple instances of module port key " +
                             "'{0}' in {1}").format(k, fname))

                    port_dict[k] = port

                db[ports_key] = port_dict

        return db

    #
    # Validate an interface database and add some default fields to
    # avoid having to check whether they are present.
    #
    def __canonicalizeDb(self):
        db = self.db
        fname = self.file_path

        # Differences between platform and AFU db
        keys_expected = ['version', 'platform-name', 'module-ports-offered']
        ports_key = 'module-ports-offered'
        if (self.db_category == 'AFU'):
            keys_expected = ['version', 'module-name', 'module-ports']
            ports_key = 'module-ports'

        for key in keys_expected:
            if (key not in db):
                self.__errorExit("{0} entry missing in {1}".format(key, fname))

        if (db['version'] != 1):
            self.__errorExit(
                ("Unsupported {0} interface dictionary version " +
                 "{1} ({2})").format(self.db_category, db['version'], fname))

        # Add empty global list of preprocessor variables to define
        # if not present.
        if ('define' not in db):
            db['define'] = []

        # Make sure AFU has a 'platform-shim-module-name'
        if (self.db_category == 'AFU'):
            if ('platform-shim-module-name' not in db):
                db['platform-shim-module-name'] = None
            if (not db['platform-shim-module-name']):
                # No platform shim supported for this top-level module class.
                # Fake the shim name to keep the platform happy but record
                # that there is no shim.
                db['platform-shim-supported'] = False
                db['platform-shim-module-name'] = db['module-name']
            else:
                db['platform-shim-supported'] = True

        # Walk the module ports list
        classes_seen = dict()
        for port in db[ports_key].values():
            # Default optional is False
            if ('optional' not in port):
                port['optional'] = False

            if (port['class'] in classes_seen):
                if (self.db_category == 'AFU'):
                    # AFU's can have only a single instance of a class
                    self.__errorExit(
                        ("multiple instances of module port class " +
                         "'{0}' in {1}").format(port['class'], fname))
                else:
                    # Platforms may have multiple instances as long as they
                    # all are optional
                    if (not classes_seen[port['class']] or
                            not port['optional']):
                        self.__errorExit(
                            ("multiple instances of module port class " +
                             "'{0}' must all be optional in {1}").format(
                                 port['class'], fname))
            classes_seen[port['class']] = port['optional']

            # Default version is 1
            if ('version' not in port):
                port['version'] = 1

            # Add a 'vector'/false key/value if it isn't defined
            if ('vector' not in port):
                port['vector'] = False

            # Add empty list of preprocessor variables to define if not present
            if ('define' not in port):
                port['define'] = []

            if (not port['vector']):
                # Can't define min/max entries unless the port is a vector
                if ('min-entries' in port or 'max-entries' in port):
                    self.__errorExit(
                        ("module port class '{0}:{1}' min/max-entries " +
                         "may not be set for non-vector types {2}").format(
                             port['class'], port['interface'], fname))

                # Define min/max entries even when the port isn't a vector
                port['min-entries'] = 1
                port['max-entries'] = 1

            else:
                # Port is a vector:

                # If min-entries isn't defined, set it to 1
                if ('min-entries' not in port):
                    port['min-entries'] = 1
                if (port['min-entries'] < 1):
                    self.__errorExit(
                        ("module port class '{0}:{1}' min-entries " +
                         "must be >= 1 in {2}").format(
                             port['class'], port['interface'], fname))

                # If max-entries isn't defined, assume the AFU can handle
                # whatever the platform offers.
                if ('max-entries' not in port):
                    if (self.db_category != 'AFU'):
                        self.__errorExit(
                            ("module port class '{0}:{1}' max-entries " +
                             "must be defined in {2}").format(
                                 port['class'], port['interface'], fname))
                    port['max-entries'] = sys.maxsize
                if (port['max-entries'] < port['min-entries']):
                    self.__errorExit(
                        ("module port class '{0}:{1}' max-entries " +
                         "must be >= min-entries in {2}").format(
                             port['class'], port['interface'], fname))

            # *** Clean up legacy AFU JSON ***

            # 'add-extra-timing-reg-stages' -> 'add-timing-reg-stages'
            if ('params' in port):
                params = port['params']
                if ('add-extra-timing-reg-stages' in params):
                    params['add-timing-reg-stages'] = \
                        params.pop('add-extra-timing-reg-stages')

    #
    # Validate a platform defaults database and add some default fields
    # to avoid having to check whether they are present.
    #
    def __canonicalizePlatformDefaultsDb(self):
        db = self.db
        fname = self.file_path

        if ('version' not in db):
            db['version'] = 1
        if (not isinstance(db['version'], int)):
            self.__errorExit(("version value ({0}) must be an integer " +
                              "({1}).").format(db['version'], fname))

        if ('module-port-params' not in db):
            db['module-port-params'] = dict()

        params = db['module-port-params']
        if (not isinstance(params, dict)):
            self.__errorExit(("module-port-params in {0} must be a " +
                              "dictionary.").format(fname))

        # Each class in module-port-params must also be a dictionary
        for c in params.keys():
            if (c == 'comment'):
                None   # Ignore comments
            else:
                if (not isinstance(params[c], dict)):
                    self.__errorExit(
                        ("class {0} in module-port-params must " +
                         "be a dictionary ({1}).").format(c, fname))
