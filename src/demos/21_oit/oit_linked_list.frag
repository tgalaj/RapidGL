#version 460

layout (early_fragment_tests) in;

struct ListNode
{
	vec4 color;
	float depth;
	int coverage;
	uint next;
};

layout (binding = 0, r32ui) uniform uimage2D head_pointers;

layout (binding = 0, std430) buffer linked_lists
{
	ListNode nodes[];
};

layout (binding = 1, std430) buffer list_info
{
	uint next_node_counter;
	uint max_nodes;
};

struct DirectionalLight
{
    vec3 color;
    float intensity;
    vec3 direction;
};

uniform DirectionalLight light;
uniform vec3 cam_pos;
uniform float transparency;

in vec3 world_pos;
in vec3 normal;

vec4 blinnPhong(DirectionalLight light, vec3 normal, vec3 world_pos)
{
    float diffuse = max(dot(normal, -normalize(light.direction)), 0.0);

    vec3 dir_to_eye  = normalize(cam_pos - world_pos);
    vec3 half_vector = normalize(dir_to_eye - light.direction);
    float specular   = pow(max(dot(half_vector, normal), 0.0), 20.0);

    vec4 diffuse_color  = vec4(light.color, 1.0) * light.intensity * diffuse;
    vec4 specular_color = vec4(1.0) * specular * 0.5;

    return diffuse_color + specular_color;
}

void main()
{
	// Get the index of the next empty slot in the buffer
	uint node_index = atomicAdd(next_node_counter, 1);

	// Is our buffer full?  If so, we don't add the fragment to the list.
	if (node_index < max_nodes)
	{
		// Our fragment will be the new head of the linked list, so
		// replace the value at gl_FragCoord.xy with our new node's
		// index.  We use imageAtomicExchange to make sure that this
		// is an atomic operation.  The return value is the old head
		// of the list (the previous value), which will become the
		// next element in the list once our node is inserted.
		uint previous_head = imageAtomicExchange(head_pointers, ivec2(gl_FragCoord.xy), node_index);

		// Here we set the color and depth of this new node to the color
		// and depth of the fragment.  The next pointer, points to the
		// previous head of the list.
		vec4 color = blinnPhong(light, normalize(normal), world_pos) + vec4(vec3(0.18), 1.0);
		color.a    = transparency;

		nodes[node_index].color    = color;
		nodes[node_index].depth    = gl_FragCoord.z;
		nodes[node_index].coverage = gl_SampleMaskIn[0];
		nodes[node_index].next     = previous_head;
	}
}