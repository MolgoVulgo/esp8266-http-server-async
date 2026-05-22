import click

from platformio.test.result import TestCase, TestStatus
from platformio.test.runners.base import TestRunnerBase


class CustomTestRunner(TestRunnerBase):
    def on_testing_line_output(self, line):
        click.echo(line, nl=False)
        text = line.strip()
        if text.startswith("OK "):
            self.test_suite.add_case(
                TestCase(text[3:], TestStatus.PASSED, stdout=text)
            )
        elif text.startswith("FAILED "):
            self.test_suite.add_case(
                TestCase(text[7:], TestStatus.FAILED, stdout=text)
            )
