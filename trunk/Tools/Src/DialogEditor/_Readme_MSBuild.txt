��� ��������� �������� ��������� ������ ������������ ���� DialogEditor.csproj.targets, ������� ��������
����������� ������ MSBuild (����� c ����� �� �����������, ��� � ��� ��������� ����� *.csproj). �� ������
������ ���� ���� ��������� �� ���� ��������, � � �� ���������� ��������� ��������� ������:
1. �������, ���� ������������ ������ ���� ��������;
2. ����� ����� ������ (���� DialogEditor.Version.cs) � ����� ��� ���� ������ ���������� (DialogEditor.AssemblyInfo.cs).

���� DialogEditor.csproj.targets ������������ ���� ��������� ����������� ����� ������� (*.csproj):

<Project>
...
  <!--��������� ������ �������� � ������� ������ ���� DialogEditor.csproj.targets. �������� ��������, ��� ���
      ����������� � ������ ������ ���� ����������� �� ����, ��� ����� ��������� ���� Microsoft.CSharp.targets-->
  <Import Condition="Exists('$(SolutionDir)$(SolutionName).csproj.targets')" Project="$(SolutionDir)$(SolutionName).csproj.targets" />

  <!--��������� ������ ������������ VS ��-��������� ��� ���� �������� �, ����� �� ��-���������, ��������� � �����
      ����� ������� (*.csproj)-->
  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
...
</Project>