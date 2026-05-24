from .code_point_properties import CodePoint, CodePointProperties

class CodePointData(dict[CodePoint, CodePointProperties]):
    def __missing__(self, code_point: CodePoint) -> CodePointProperties:
        properties = CodePointProperties.default_properties_for_code_point(code_point)

        self[code_point] = properties

        return properties