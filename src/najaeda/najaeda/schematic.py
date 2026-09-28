# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Optional schematic viewer: https://github.com/najaeda/naja-schematic."""

try:
    from naja_schematic import show, diagnosis_response, handle_request, __version__
except ImportError as e:
    raise ImportError(
        'The schematic viewer requires pip install "najaeda[schematic]"'
    ) from e

__all__ = ["show", "diagnosis_response", "handle_request", "__version__"]
