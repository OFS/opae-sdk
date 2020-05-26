#!/usr/bin/env python3
# Copyright(c) 2020, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import os
import re
import subprocess
import sys
import time


class GitCommit:
    GIT_REFS_PATTERN = r'(?:\s*tag:)(?:\s+(?P<tag>[^),]+))'
    GIT_REFS_RE = re.compile(GIT_REFS_PATTERN)

    def __init__(self, repo, githash):
        self._repo = repo
        self._hash = githash

    def __eq__(self, other):
        if isinstance(other, GitCommit):
            other = other.hash
        return self.hash == other

    def __ne__(self, other):
        if isinstance(other, GitCommit):
            other = other.hash
        return self.hash != other

    def __str__(self):
        return self.hash

    @property
    def hash(self):
        return self._hash

    @property
    def abbrev_hash(self):
        return self._hash[:7]

    @property
    def subject(self):
        return self._repo._run_log_cmd('%s', self._hash)

    @property
    def author(self):
        return self._repo._run_log_cmd('%aN', self._hash)

    @property
    def author_email(self):
        return self._repo._run_log_cmd('%aE', self._hash)

    @property
    def tags(self):
        refs = self._repo._run_log_cmd('%d', self._hash)
        refs = refs.split(',')
        t = []
        for r in refs:
            if r.startswith('('):
                r = r[1:]
            elif r.endswith(')'):
                r = r[:len(r)-1]
            m = self.GIT_REFS_RE.match(r)
            if m:
                t.append(m.group('tag'))
        return t

    def changelog(self, formatter, substitutions):
        return formatter.format(self, substitutions)


class GitRepo:
    def __init__(self, gitdir):
        self._gitdir = gitdir

    @property
    def gitdir(self):
        return self._gitdir

    @staticmethod
    def find_repo(path):
        if path and path != os.path.sep:
            gitdir = os.path.join(path, '.git')
            if os.path.isdir(gitdir):
                return GitRepo(gitdir)
            else:
                return GitRepo.find_repo(os.path.split(path)[0])

    def _run_process(self, cmd):
        return subprocess.check_output(cmd,
                                       stderr=open('/dev/null', 'w')
                                       ).decode('UTF-8').strip()

    def _run_log_cmd(self, fmt, *args):
        cmd = ['git', '--git-dir={}'.format(self._gitdir),
               '--no-pager', 'log', '-1', '--format={}'.format(fmt)]
        if args:
            cmd.append(*args)
        return self._run_process(cmd)

    def get_commit(self, commitish):
        try:
            githash = self._run_log_cmd('%H', commitish)
        except subprocess.CalledProcessError:
            return None
        return GitCommit(self, githash)

    def last_tag(self):
        '''Retrieves the name of the last tag on the current branch.'''
        cmd = ['git', '--git-dir={}'.format(self._gitdir),
               '--no-pager', 'describe', '--tags', '--abbrev=0']
        try:
            return self._run_process(cmd)
        except subprocess.CalledProcessError:
            return None


class DebianChangelogFormatter:
    HEADER_FORMAT = '{project} ({version}) {stability}; urgency={urgency}\n\n'
    FOOTER_FORMAT = '\n -- {author} <{email}>  {date}\n'

    def header(self, substitutions):
        return self.HEADER_FORMAT.format(**substitutions)

    def format(self, commit, substitutions):
        return '  * {} {}\n'.format(commit.abbrev_hash,
                                    commit.subject)

    def footer(self, substitutions):
        return self.FOOTER_FORMAT.format(**substitutions)

    def date(self, tm):
        return time.strftime('%a, %d %b %Y %H:%M:%S %z', tm)


class RedHatChangelogFormatter:
    HEADER_FORMAT = '* {date} {author} <{email}> - {version}\n'

    def header(self, substitutions):
        return self.HEADER_FORMAT.format(**substitutions)

    def format(self, commit, substitutions):
        return '- {}\n'.format(commit.subject)

    def footer(self, substitutions):
        return ''

    def date(self, tm):
        return time.strftime('%a %b %d %Y', tm)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('format', choices=['deb', 'rpm'],
                        nargs='?', default='rpm', help='Desired format')
    parser.add_argument('--git-dir', default=None,
                        help='Path to .git/')
    parser.add_argument('--project', default='opae',
                        help='Project name [%(default)s]')
    parser.add_argument('--project-version', default='1.0.0',
                        help='Project version [%(default)s]')
    parser.add_argument('--stability', default='stable',
                        help='Project stability [%(default)s]')
    parser.add_argument('--urgency', default='medium',
                        help='Project urgency [%(default)s]')
    parser.add_argument('--author', default='The OPAE Dev Team',
                        help='Project author [%(default)s]')
    parser.add_argument('--email', default='opae@lists.01.org',
                        help='Project email [%(default)s]')
    return parser.parse_args()


def main():
    args = parse_args()

    if args.git_dir and os.path.isdir(args.git_dir):
        repo = GitRepo(args.git_dir)
    else:
        repo = GitRepo.find_repo(os.getcwd())

    if not repo:
        print('No git repo found.')
        sys.exit(1)

    formatters = {'deb': DebianChangelogFormatter,
                  'rpm': RedHatChangelogFormatter}
    formatter = formatters[args.format]()

    subs = {'project': args.project,
            'version': args.project_version,
            'stability': args.stability,
            'urgency': args.urgency,
            'author': args.author,
            'email': args.email,
            'date': formatter.date(time.localtime())}

    head = repo.get_commit('HEAD')

    # If HEAD has tags, then output one changelog entry for HEAD.
    if head.tags:
        print(formatter.header(subs), end='')
        print(head.changelog(formatter, subs), end='')
        print(formatter.footer(subs), end='')
    else:
        # No tag at HEAD.
        # If there is a tag on the branch, output a changelog for HEAD
        # down to, but not including, the last tagged commit.
        last_tag = repo.last_tag()
        if last_tag:
            begin = head
            end = repo.get_commit(last_tag)

            print(formatter.header(subs), end='')

            while begin != end:
                print(begin.changelog(formatter, subs), end='')
                begin = repo.get_commit('{}~'.format(begin.hash))

            print(formatter.footer(subs), end='')
        else:
            # No tag at HEAD.
            # No tag on current branch.
            # Output a changelog for HEAD.
            print(formatter.header(subs), end='')
            print(head.changelog(formatter, subs), end='')
            print(formatter.footer(subs), end='')


if __name__ == '__main__':
    main()
