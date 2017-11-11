#!/usr/bin/python

import paramiko
import sys
import select

class Remote:
    def __init__(self, host, username, password = None):
        try:
            self.ssh = paramiko.SSHClient()
            self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.ssh.connect(host, username=username, password=password)

        except paramiko.AuthenticationException:
            print("Authentication failed when connecting to %s" % host)
            sys.exit(1)

        except:
            print("Could not SSH to %s, error: %s" % host, sys.exc_info()[0])
            sys.exit(1)

    def dump_channel(self, channel):

        result = False

        while channel.recv_stderr_ready():
            rl, wl, xl = select.select([channel], [], [], 0.0)
            if len(rl) > 0:
                tmp = channel.recv_stderr(1024)
                output = tmp.decode()

                print(output, end='')
                result = True

        while channel.recv_ready():
            rl, wl, xl = select.select([channel], [], [], 0.0)
            if len(rl) > 0:
                tmp = channel.recv(1024)
                output = tmp.decode()

                print(output, end='')
                result = True

        return result

    def execute_command(self, command):

        stdin, stdout, stderr = self.ssh.exec_command(command)

        # Wait for the command to terminate
        while not stdout.channel.exit_status_ready():
            self.dump_channel(stdout.channel)

        # Empty the channels
        while self.dump_channel(stdout.channel):
            pass

    def execute_commands(self, commands):

        result = ''

        for command in commands:
            result += command + '; '

        self.execute_command(result)


    def close(self):
        self.ssh.close()


def remote_build(user, password, path):
    connection = Remote('localhost', user, password)

    try:
        connection.execute_commands([
            'cd {0}'.format(path),
            'mkdir -p build',
            'cd ./build',
            'bash -e ./../scripts/cmake.sh || exit $?',
            'make || exit $?',
            './../scripts/initrd.py || exit $?',
            './../scripts/make_image.py || exit $?'
        ])

    except:
        pass

    connection.close()


def main():

    if 3 < len(sys.argv) < 4:

        print('Usage: path user [password]')
        return

    path = sys.argv[1]
    user = sys.argv[2]
    password = None if len(sys.argv) == 3 else sys.argv[3]

    remote_build(user, password, path)


if(__name__ == '__main__'):
    main()
